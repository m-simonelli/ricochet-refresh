/* Ricochet Refresh - https://ricochetrefresh.net/
 * Copyright (C) 2020, Blueprint For Free Speech <ricochet@blueprintforfreespeech.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following disclaimer
 *      in the documentation and/or other materials provided with the
 *      distribution.
 *
 *    * Neither the names of the copyright owners nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "protocol/FileChannel.h"
#include "Channel_p.h"
#include "Connection.h"
#include "utils/SecureRNG.h"
#include "utils/Useful.h"
#include "precomp.h"
#include "context.hpp"
#include "error.hpp"
#include "globals.hpp"

using namespace Protocol;

FileChannel::FileChannel(Direction direction, Connection *connection)
    : Channel(QStringLiteral("im.ricochet.file-transfer"), direction, connection)
{
}

FileChannel::FileId FileChannel::nextFileId() {
    return ++file_id;
}

bool FileChannel::allowInboundChannelRequest(
    __attribute__((unused)) const Data::Control::OpenChannel *request, 
    Data::Control::ChannelResult *result)
{
    if (connection()->purpose() != Connection::Purpose::KnownContact) {
        qDebug() << "Rejecting request for" << type() << "channel from connection with purpose" << int(connection()->purpose());
        result->set_common_error(Data::Control::ChannelResult::UnauthorizedError);
        return false;
    }

    if (connection()->findChannel<FileChannel>(Channel::Inbound)) {
        qDebug() << "Rejecting request for" << type() << "channel because one is already open";
        return false;
    }

    return true;
}

bool FileChannel::allowOutboundChannelRequest(
    __attribute__((unused)) Data::Control::OpenChannel *request)
{
    if (connection()->findChannel<FileChannel>(Channel::Outbound)) {
        BUG() << "Rejecting outbound request for" << type() << "channel because one is already open on this connection";
        return false;
    }

    if (connection()->purpose() != Connection::Purpose::KnownContact) {
        BUG() << "Rejecting outbound request for" << type() << "channel for connection with unexpected purpose" << int(connection()->purpose());
        return false;
    }

    return true;
}

void FileChannel::receivePacket(const QByteArray &packet)
{
    Data::File::Packet message;
    if (!message.ParseFromArray(packet.constData(), packet.size())) {
        closeChannel();
        return;
    }

    if (message.has_file_header()) {
        handleFileHeader(message.file_header());
    } else if (message.has_file_chunk()) {
        handleFileChunk(message.file_chunk());
    } else if (message.has_file_ack()) {
        handleFileAck(message.file_ack());
    } else if (message.has_file_header_ack()) {
        handleFileHeaderAck(message.file_header_ack());    
    } else {
        qWarning() << "Unrecognized file packet on " << type();
        closeChannel();
    }
}

void FileChannel::handleFileHeader(const Data::File::FileHeader &message){
    std::unique_ptr<Data::File::FileHeaderAck> response = std::make_unique<Data::File::FileHeaderAck>();
    std::filesystem::path dirname;
    std::fstream hdr_file;
    Data::File::Packet packet;

    if (direction() != Inbound) {
        qWarning() << "Rejected inbound message (FileHeader) on an outbound channel";
        response->set_accepted(false);
        response->set_reason(Data::File::FileHeaderAck_Reason::FileHeaderAck_Reason_HEADER_ACK_REFUSE_OTHER);
    } else if (!message.has_size() || !message.has_chunk_count()) {
        /* rationale:
         *  - if there's no size, we know when we've reached the end when cur_chunk == n_chunks
         *  - if there's no chunk count, we know when we've reached the end when total_bytes >= size
         *  - if there's neither, size cannot be determined */
        /* TODO: given ^, are both actually needed? */
        qWarning() << "Rejected file header with missing size";
        response->set_accepted(false);
        response->set_reason(Data::File::FileHeaderAck_Reason::FileHeaderAck_Reason_HEADER_ACK_REFUSE_NO_SIZE);
    } else if (!message.has_sha3_512()) {
        qWarning() << "Rejected file header with missing hash (sha3_512) - cannot validate";
        response->set_accepted(false);
        response->set_reason(Data::File::FileHeaderAck_Reason::FileHeaderAck_Reason_HEADER_ACK_REFUSE_NO_SHA3_512);
    } else if (!message.has_file_id()) {
        qWarning() << "Rejected file header with missing id";
        response->set_accepted(false);
        response->set_reason(Data::File::FileHeaderAck_Reason::FileHeaderAck_Reason_HEADER_ACK_REFUSE_OTHER);
    } else {
        response->set_accepted(true);
        response->set_reason(Data::File::FileHeaderAck_Reason::FileHeaderAck_Reason_HEADER_ACK_ACCEPT);
        /* Use the file id as part of the directory name */
        /* TODO: windows */
        dirname  = "/tmp/ricochet-";
        dirname += connection()->serverHostname().remove(".onion").toStdString();
        dirname += "-";
        dirname += std::to_string(message.file_id());

        /* create directory to store chunks in /tmp */
        try {
            std::filesystem::create_directory(dirname);
        } catch (std::filesystem::filesystem_error &e) {
            qWarning() << "Could not create tmp directory, reason: " << e.what();
            response->set_accepted(false);
        }

        hdr_file.open(dirname/"hdr_file", std::ios::out | std::ios::binary);
        if (!hdr_file) {
            qWarning() << "Could not open header file";
            response->set_accepted(false);
        } else {
            auto size = message.size();
            auto chunk_count = message.chunk_count();
            auto sha3_512 = message.sha3_512();
            hdr_file.write(reinterpret_cast<const char *>(&chunk_count), sizeof(chunk_count))
                    .write(reinterpret_cast<const char *>(&size), sizeof(size))
                    .write(reinterpret_cast<const char *>(sha3_512.c_str()), sha3_512.size());

            if (message.has_name()) {
                hdr_file << message.name().length()
                         << message.name();
            } else {
                hdr_file << std::to_string(message.file_id()).length()
                         << std::to_string(message.file_id());
            }
        }
    }

    if (message.has_file_id()) {
        /* send the response */
        response->set_file_id(message.file_id());
        packet.set_allocated_file_header_ack(std::move(response).get());
        Channel::sendMessage(packet);
    }
}

void FileChannel::handleFileChunk(__attribute__((unused)) const Data::File::FileChunk &message){
    /* not implemented yet, this should never be called as of now */
    TEGO_THROW_IF_FALSE(false);
}

void FileChannel::handleFileAck(__attribute__((unused)) const Data::File::FileChunkAck &message){
    /* not implemented yet, this should never be called as of now */
    TEGO_THROW_IF_FALSE(false);
}

void FileChannel::handleFileHeaderAck(__attribute__((unused)) const Data::File::FileHeaderAck &message){
    /* not implemented yet, this should never be called as of now */
    TEGO_THROW_IF_FALSE(false);
}

bool FileChannel::sendFileWithId(QString file_url, 
                                 __attribute__((unused)) QDateTime time, 
                                 FileId id) {
    std::fstream file;
    std::fstream chunk;
    std::uintmax_t file_size;
    std::uintmax_t file_chunks;
    std::filesystem::path file_path;
    std::filesystem::path file_dir;
    std::string chunk_name;
    Data::File::FileHeader header;
    char *buf;
    std::streamsize bytes_read;
    EVP_MD_CTX *sha3_512_ctx;
    unsigned char sha3_512_value[EVP_MAX_MD_SIZE];
    unsigned int sha3_512_value_len;

    if (direction() != Outbound) {
        BUG() << "Attempted to send outbound message on non outbound channel";
        return false;
    }

    if (file_url.isEmpty()) {
        BUG() << "File URL is empty, this should never have been reached";
        return false;
    }

    /* sendNextChunk will resume a transfer if connection was interrupted */
    if (sendNextChunk(id)) return true;

    file_path = file_url.toStdString();

    /* only allow regular files or symlinks to regular files (no symlink chaining) */
    if (!std::filesystem::exists(file_path) || 
        !std::filesystem::is_regular_file(file_path)) {
        file_path = std::filesystem::read_symlink(file_path);
        if (!std::filesystem::is_regular_file(file_path)) {
            /* TODO: is there a good way to follow a symlink chain without ending up in a recursion loop? */
            qWarning() << "Rejected file url, reason: More than 1 level of symlink chaining";
            return false;
        }
    }

    try {
        file_size = std::filesystem::file_size(file_path);
    } catch (std::filesystem::filesystem_error &e) {
        qWarning() << "Rejected file url, reason: " << e.what();
        return false;
    }

    file_chunks = CEIL_DIV(file_size, FileMaxChunkSize);
    
    file.open(file_path, std::ios::in | std::ios::binary);
    if (!file) {
        qWarning() << "Failed to open file for sending header";
        return false;
    }

    /* this amount of bytes is completly arbitrary, but keeping it rather low
     * helps with not using too much memory */
    buf = new char[65535]();

    /* Review this */
    sha3_512_ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(sha3_512_ctx, EVP_sha3_512(), NULL);
    while (file.good()) {
        file.read(buf, 65535);
        bytes_read = file.gcount();
        /* the only time bytes_read would be >65535 is if gcount thinks the
         * amount of bytes read is unrepresentable, in which case something has
         * gone wrong */
        TEGO_THROW_IF_TRUE_MSG(bytes_read > 65535, "Invalid amount of bytes read");
        EVP_DigestUpdate(sha3_512_ctx, buf, bytes_read);
    }
    EVP_DigestFinal_ex(sha3_512_ctx, sha3_512_value, &sha3_512_value_len);
    EVP_MD_CTX_free(sha3_512_ctx);

    delete[] buf;
    file.close();

    header.set_file_id(nextFileId());
    header.set_size(file_size);
    header.set_chunk_count(file_chunks);
    header.set_sha3_512(sha3_512_value, sha3_512_value_len);

    Channel::sendMessage(header);
    
    /* the first chunk will get sent after the first header ack */
    return true;
}

bool FileChannel::sendNextChunk(FileId id) {
    //TODO: check either file digest or file last modified time, if they don't match before, start from chunk 0
    auto it =
        std::find_if(queuedFiles.begin(), 
        queuedFiles.end(), 
        [id](const queuedFile &qf) { return qf.id == id; });

    if (it == queuedFiles.end())
        return false;

    return sendChunkWithId(id, it->path, it->last_chunk++);
}

bool FileChannel::sendChunkWithId(FileId fid, ChunkId cid) {
    auto it = 
        std::find_if(queuedFiles.begin(),
        queuedFiles.end(),
        [fid](const queuedFile &qf) {return qf.id == fid; });

    if (it == queuedFiles.end())
        return false;

    return sendChunkWithId(fid, it->path, cid);
}

bool FileChannel::sendChunkWithId(FileId fid, std::filesystem::path &fpath, ChunkId cid) {
    std::fstream file;
    Data::File::FileChunk chunk;
    char *buf;
    std::streamsize bytes_read;
    EVP_MD_CTX *sha3_512_ctx;
    unsigned char sha3_512_value[EVP_MAX_MD_SIZE];
    unsigned int sha3_512_value_len;


    file.open(fpath, std::ios::in | std::ios::binary);
    if (!file) {
        qWarning() << "Failed to open file for sending chunk";
        return false;
    }

    if (FileMaxChunkSize * cid > std::filesystem::file_size(fpath)) {
        qWarning() << "Attempted to read chunk beyond EOF";
        return false;
    }
    
    /* go to the pos of the chunk */
    file.seekg(FileMaxChunkSize * cid);
    if (!file) {
        qWarning() << "Failed to seek to last position in file for chunking";
        return false;
    }

    buf = new char[FileMaxChunkSize]();
    
    file.read(buf, FileMaxChunkSize);
    bytes_read = file.gcount();
    /* the only time bytes_read would be >65535 is if gcount thinks the
     * amount of bytes read is unrepresentable, in which case something has
     * gone wrong */
    TEGO_THROW_IF_TRUE_MSG(bytes_read > 65535, "Invalid amount of bytes read");
    
    /* hash this chunk */
    sha3_512_ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(sha3_512_ctx, EVP_sha3_512(), NULL);
    EVP_DigestUpdate(sha3_512_ctx, buf, bytes_read);
    EVP_DigestFinal_ex(sha3_512_ctx, sha3_512_value, &sha3_512_value_len);
    EVP_MD_CTX_free(sha3_512_ctx);

    /* send this chunk */
    chunk.set_sha3_512(sha3_512_value, sha3_512_value_len);
    chunk.set_file_id(fid);
    chunk.set_chunk_id(cid);
    chunk.set_chunk_size(bytes_read);
    chunk.set_chunk_data(buf, bytes_read);
    //TODO chunk.set_time_delta();

    delete[] buf;
    file.close();

    Channel::sendMessage(chunk);
    return true;
}
