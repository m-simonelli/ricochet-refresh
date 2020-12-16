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

using namespace Protocol;

FileChannel::FileChannel(Direction direction, Connection *connection)
    : Channel(QStringLiteral("im.ricochet.file-transfer"), direction, connection)
{
}

/* XXX: remove this once the below functions are implemented */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
FileChannel::FileId FileChannel::nextFileId() {
    return ++file_id;
}

bool FileChannel::allowInboundChannelRequest(const Data::Control::OpenChannel *request, Data::Control::ChannelResult *result)
{
    return false;
}

bool FileChannel::allowOutboundChannelRequest(Data::Control::OpenChannel *request)
{
    return false;
}

void FileChannel::receivePacket(const QByteArray &packet)
{
    return;
}
#pragma GCC diagnostic pop

bool FileChannel::sendFile(QString file_url, QDateTime time, FileId &id) {
    id = nextFileId();
    return sendFileWithId(file_url, time, id);
}

bool FileChannel::sendFileWithId(QString file_url, 
                                 __attribute__((unused)) QDateTime time, 
                                 __attribute__((unused)) FileId &id) {
    std::fstream file;
    std::fstream chunk;
    std::uintmax_t file_size;
    std::uintmax_t file_chunks;
    std::uintmax_t cur_chunk;
    std::uintmax_t chunk_name_sz;
    std::filesystem::path file_path;
    std::filesystem::path file_dir;
    std::string chunk_name;
    char *file_chunk = new char[FileMaxDecodedChunkSize];

    if (direction() != Outbound) {
        BUG() << "Attempted to send outbound message on non outbound channel";
        return false;
    }

    if (file_url.isEmpty()) {
        BUG() << "File URL is empty, this should never have been reached";
        return false;
    }

    file_path = file_url.toStdString();

    /* only allow regular files or symlinks to regular files (no symlink chaining) */
    if (!std::filesystem::exists(file_path) || 
        !std::filesystem::is_regular_file(file_path)) {
        file_path = std::filesystem::read_symlink(file_path);
        if (!std::filesystem::is_regular_file(file_path)) {
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
    file_chunks = CEIL_DIV(file_size, 1500);

    /* XXX: write to /tmp/ricochet/... instead of wherever the file is */
    /* get parent path, make directory there called "chunks", chunk the file into
     * 1500 byte blocks */
    file_dir = file_path.parent_path();

    try {
        std::filesystem::create_directory(file_dir/"chunks");
    } catch (std::filesystem::filesystem_error &e) {
        qWarning() << "Could not create directory, reason: " << e.what();
        return false;
    }

    file.open(file_path);

    /* XXX: should it be file_id-%d.part? */
    /* chunk name is formatted as:
     * filename-%d.part */
    chunk_name_sz = file_path.filename().string().length() + 
                      std::to_string(file_chunks).length() +
                      strlen("-.part");
    chunk_name.resize(chunk_name_sz);

    for (cur_chunk = 0; cur_chunk < file_chunks; cur_chunk++) {
#if __cplusplus < 201103L
    /* NOTE: I'm not sure if anything else relies on newer versions of the C++ standard */
    #error "The set C++ standard is too old, and will lead to undefined behaviour. Please use at least C++11"
#endif
        /* get the chunk name */
        /* snprintf to &std::string[0] is valid as of c++11, pre c++11 this was UB */
        snprintf(&chunk_name[0], chunk_name_sz, "%s-%0*ld.part", 
                 file_path.filename().string().c_str(), 
                 (int)std::to_string(file_chunks).length(), 
                 cur_chunk);
        file.read(file_chunk, FileMaxDecodedChunkSize);

        /* write out the chunk */
        chunk.open(file_path/"chunks"/chunk_name, std::ios::out | std::ios::binary);
        chunk.write(file_chunk, FileMaxDecodedChunkSize);
        chunk.close();
    }

    return false;
}