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

#include "FileChannel.h"
#include "Channel_p.h"
#include "Connection.h"
#include "utils/SecureRNG.h"
#include "utils/Useful.h"
#include "context.hpp"
#include "error.hpp"
#include "globals.hpp"

using namespace Protocol;

FileChannel::FileChannel(Direction direction, Connection *connection)
    : Channel(QStringLiteral("im.ricochet.file-transfer"), direction, connection)
{
}

//unused
FileChannel::file_id_t FileChannel::nextFileId() {
    return ++file_id;
}

size_t FileChannel::fsize_to_chunks(size_t sz) {
    return (sz + (FileMaxChunkSize - 1)) / FileMaxChunkSize;
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
    } else if (message.has_file_chunk_ack()) {
        handleFileChunkAck(message.file_chunk_ack());
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
    pendingRecvFile prf;

    if (direction() != Inbound) {
        qWarning() << "Rejected inbound message (FileHeader) on an outbound channel";
        response->set_accepted(false);
    } else if (!message.has_size() || !message.has_chunk_count()) {
        /* rationale:
         *  - if there's no size, we know when we've reached the end when cur_chunk == n_chunks
         *  - if there's no chunk count, we know when we've reached the end when total_bytes >= size
         *  - if there's neither, size cannot be determined */
        /* TODO: given ^, are both actually needed? */
        qWarning() << "Rejected file header with missing size";
        response->set_accepted(false);
    } else if (!message.has_sha3_512()) {
        qWarning() << "Rejected file header with missing hash (sha3_512) - cannot validate";
        response->set_accepted(false);
    } else if (!message.has_file_id()) {
        qWarning() << "Rejected file header with missing id";
        response->set_accepted(false);
    } else {
        response->set_accepted(true);
        /* Use the file id and onion url as part of the directory name */
        dirname = fmt::format("{}/ricochet-{}-{}",
                                QStandardPaths::TempLocation,
                                connection()->serverHostname().remove(".onion").toStdString(),
                                message.file_id());

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

            prf.id = message.file_id();
            prf.path = dirname;
            prf.size = size;
            prf.cur_chunk = 0;
            pendingRecvFiles.push_back(prf);
        }
    }

    if (message.has_file_id()) {
        /* send the response */
        response->set_file_id(message.file_id());
        packet.set_allocated_file_header_ack(std::move(response).get());
        Channel::sendMessage(packet);
    }
}

void FileChannel::handleFileChunk(const Data::File::FileChunk &message){
    EVP_MD_CTX *sha3_512_ctx;
    unsigned char sha3_512_value[EVP_MAX_MD_SIZE];
    unsigned int sha3_512_value_len;

    std::filesystem::path fpath;
    std::ofstream chunk_file;

    auto it =
        std::find_if(pendingRecvFiles.begin(),
        pendingRecvFiles.end(),
        [message](const pendingRecvFile &prf) { return prf.id == message.file_id(); });

    if (it == pendingRecvFiles.end())
        return;

    if (message.chunk_size() > FileMaxChunkSize || message.chunk_size() != message.chunk_data().length())
        return; /* either chunk too large or the size given was invalid */

    sha3_512_ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(sha3_512_ctx, EVP_sha3_512(), NULL);
    EVP_DigestUpdate(sha3_512_ctx, message.chunk_data().c_str(), message.chunk_size());
    EVP_DigestFinal_ex(sha3_512_ctx, sha3_512_value, &sha3_512_value_len);
    EVP_MD_CTX_free(sha3_512_ctx);

    if (strncmp(reinterpret_cast<const char*>(sha3_512_value), message.sha3_512().c_str(), sha3_512_value_len) != 0)
        return; /* sha3_512 mismatch */

    fpath = it->path / std::to_string(message.chunk_id());
    /* open (and clear contents if they exist) */
    chunk_file.open(fpath, std::ios::out | std::ios::binary | std::ios::trunc);
    chunk_file.write(message.chunk_data().c_str(), message.chunk_size());
    chunk_file.close();
}

void FileChannel::handleFileChunkAck(const Data::File::FileChunkAck &message){
    auto it =
        std::find_if(queuedFiles.begin(),
        queuedFiles.end(),
        [message](const queuedFile &qf) { return qf.cur_chunk == message.file_chunk_id(); });

    if (it == queuedFiles.end()) {
        qWarning() << "recieved ack for a chunk we never sent";
        return;
    }

    if (it->cur_chunk * FileMaxChunkSize > std::filesystem::file_size(it->path)) {
        it->finished = true;
    }

    if (it->finished) {
        auto index = std::distance(queuedFiles.begin(), it);
        queuedFiles.erase(queuedFiles.begin() + index);
        return;
    }

    //todo: don't infinitely resend a chunk if it infinitely gets declined
    if (message.accepted()) it->cur_chunk++;
    sendChunkWithId(it->id, it->path, it->cur_chunk);
}

void FileChannel::handleFileHeaderAck(const Data::File::FileHeaderAck &message){
    if (direction() != Inbound) {
        qWarning() << "Rejected inbound message on outbound file channel";
        return;
    }

    auto it =
        std::find_if(pendingFileHeaders.begin(), 
        pendingFileHeaders.end(), 
        [message](const queuedFile &qf) { return qf.id == message.file_id(); });

    if (it == pendingFileHeaders.end()) {
        qWarning() << "recieved ack for a file header we never sent";
        return;
    }

    auto index = std::distance(pendingFileHeaders.begin(), it);

    if (message.accepted()) {
        queuedFiles.insert(queuedFiles.end(), std::make_move_iterator(pendingFileHeaders.begin() + index),
                                              std::make_move_iterator(pendingFileHeaders.end()));
    }

    pendingFileHeaders.erase(pendingFileHeaders.begin() + index);

    //todo: message_acknowledged signal/callback needs to go here, before the call to sendChunkWithId

    /* start the transfer at chunk 0 */
    if (message.accepted()) {
        sendChunkWithId(it->id, it->path, 0);
    }
}

bool FileChannel::sendFileWithId(QString file_uri,
                                 QDateTime,
                                 file_id_t id) {
    std::ifstream file;
    std::uintmax_t file_chunks;
    std::filesystem::path file_path;
    std::filesystem::path file_dir;
    std::string chunk_name;
    uint64_t file_size;

    file_id_t file_id;
    queuedFile qf;
    Data::File::FileHeader header;

    std::streamsize bytes_read;
    
    EVP_MD_CTX *sha3_512_ctx;
    unsigned char sha3_512_value[EVP_MAX_MD_SIZE];
    unsigned int sha3_512_value_len;

    if (direction() != Outbound) {
        BUG() << "Attempted to send outbound message on non outbound channel";
        return false;
    }

    if (file_uri.isEmpty()) {
        BUG() << "File URI is empty, this should never have been reached";
        return false;
    }

    /* sendNextChunk will resume a transfer if connection was interrupted */
    if (sendNextChunk(id)) return true;

    file_path = file_uri.toStdString();

    /* only allow regular files or symlinks chains to regular files */
    try {
        file_path = std::filesystem::canonical(file_path);
    } catch (std::filesystem::filesystem_error &e) {
        qWarning() << "Could not resolve symlink path/file path, reason: " << e.what();
        return false;
    }

    try {
        file_size = std::filesystem::file_size(file_path);
    } catch (std::filesystem::filesystem_error &e) {
        qWarning() << "Rejected file URI, reason: " << e.what();
        return false;
    }

    file_chunks = fsize_to_chunks(file_size);
    
    file.open(file_path, std::ios::in | std::ios::binary);
    if (!file) {
        qWarning() << "Failed to open file for sending header";
        return false;
    }

    /* this amount of bytes is completly arbitrary, but keeping it rather low
     * helps with not using too much memory */
    auto buf = std::make_unique<char[]>(SHA3_512_BUFSIZE);

    /* Review this */
    sha3_512_ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(sha3_512_ctx, EVP_sha3_512(), NULL);
    while (file.good()) {
        file.read(buf.get(), SHA3_512_BUFSIZE);
        bytes_read = file.gcount();
        /* the only time bytes_read would be >SHA3_512_BUFSIZE is if gcount thinks the
         * amount of bytes read is unrepresentable, in which case something has
         * gone wrong */
        TEGO_THROW_IF_TRUE_MSG(bytes_read > SHA3_512_BUFSIZE, "Invalid amount of bytes read");
        EVP_DigestUpdate(sha3_512_ctx, buf.get(), bytes_read);
    }
    EVP_DigestFinal_ex(sha3_512_ctx, sha3_512_value, &sha3_512_value_len);
    EVP_MD_CTX_free(sha3_512_ctx);

    file.close();

    file_id = nextFileId();
    qf.id = file_id;
    qf.path = file_path;
    qf.cur_chunk = 0;
    qf.peer_did_accept = false;
    qf.size = file_size;

    pendingFileHeaders.push_back(qf);

    header.set_file_id(file_id);
    header.set_size(file_size);
    header.set_chunk_count(file_chunks);
    header.set_sha3_512(sha3_512_value, sha3_512_value_len);

    Channel::sendMessage(header);
    
    /* the first chunk will get sent after the first header ack */
    return true;
}

bool FileChannel::sendNextChunk(file_id_t id) {
    //TODO: check either file digest or file last modified time, if they don't match before, start from chunk 0
    auto it =
        std::find_if(queuedFiles.begin(), 
        queuedFiles.end(), 
        [id](const queuedFile &qf) { return qf.id == id; });

    if (it == queuedFiles.end()) return false;
    if (it->cur_chunk * FileMaxChunkSize > std::filesystem::file_size(it->path)) it->finished = true;
    if (it->finished) return false;

    return sendChunkWithId(id, it->path, it->cur_chunk++);
}

bool FileChannel::sendChunkWithId(file_id_t fid, std::filesystem::path &fpath, chunk_id_t cid) {
    std::ifstream file;
    Data::File::FileChunk chunk;
    std::streamsize bytes_read;
    EVP_MD_CTX *sha3_512_ctx;
    unsigned char sha3_512_value[EVP_MAX_MD_SIZE];
    unsigned int sha3_512_value_len;

    if (direction() != Outbound) {
        BUG() << "Attempted to send outbound message on non outbound channel";
        return false;
    }

    file.open(fpath, std::ios::in | std::ios::binary);
    if (!file) {
        qWarning() << "Failed to open file for sending chunk";
        return false;
    }

    if (FileMaxChunkSize * (cid + 1) > std::filesystem::file_size(fpath)) {
        qWarning() << "Attempted to start read beyond eof";
    }
    
    /* go to the pos of the chunk */
    file.seekg(FileMaxChunkSize * cid);
    if (!file) {
        qWarning() << "Failed to seek to last position in file for chunking";
        return false;
    }

    auto buf = std::make_unique<char[]>(FileMaxChunkSize);
    
    file.read(buf.get(), FileMaxChunkSize);
    bytes_read = file.gcount();
    /* the only time bytes_read would be >65535 is if gcount thinks the
     * amount of bytes read is unrepresentable, in which case something has
     * gone wrong */
    TEGO_THROW_IF_TRUE_MSG(bytes_read > 65535, "Invalid amount of bytes read");
    
    /* hash this chunk */
    /* review this */
    sha3_512_ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(sha3_512_ctx, EVP_sha3_512(), NULL);
    EVP_DigestUpdate(sha3_512_ctx, buf.get(), bytes_read);
    EVP_DigestFinal_ex(sha3_512_ctx, sha3_512_value, &sha3_512_value_len);
    EVP_MD_CTX_free(sha3_512_ctx);

    /* send this chunk */
    chunk.set_sha3_512(sha3_512_value, sha3_512_value_len);
    chunk.set_file_id(fid);
    chunk.set_chunk_id(cid);
    chunk.set_chunk_size(bytes_read);
    chunk.set_chunk_data(buf.get(), bytes_read);
    //TODO chunk.set_time_delta();

    file.close();

    Channel::sendMessage(chunk);
    return true;
}
