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

#ifndef PROTOCOL_FILECHANNEL_H
#define PROTOCOL_FILECHANNEL_H

#include "protocol/Channel.h"
#include "protocol/FileChannel.pb.h"

namespace Protocol
{

class FileChannel : public Channel
{
    Q_OBJECT;
    Q_DISABLE_COPY(FileChannel);

public:
    typedef quint32 file_id_t;
    typedef quint32 chunk_id_t;
    constexpr static int FileMaxChunkSize = 2000;
    constexpr static int SHA3_512_BUFSIZE = 65535;

    explicit FileChannel(Direction direction, Connection *connection);

    bool sendFileWithId(QString file_url, QDateTime time, file_id_t id);
    bool sendNextChunk(file_id_t id);
    bool sendChunkWithId(file_id_t fid, std::filesystem::path &fpath, chunk_id_t cid);

signals:
protected:
    virtual bool allowInboundChannelRequest(const Data::Control::OpenChannel *request, Data::Control::ChannelResult *result);
    virtual bool allowOutboundChannelRequest(Data::Control::OpenChannel *request);
    virtual void receivePacket(const QByteArray &packet);
private:
    file_id_t nextFileId();
    file_id_t file_id;
    size_t fsize_to_chunks(size_t sz);
    
    struct queuedFile {
        file_id_t id;
        std::filesystem::path path;
        size_t size;
        chunk_id_t cur_chunk;
        bool finished;
        bool peer_did_accept;
    };

    struct pendingRecvFile {
        file_id_t id;
        std::filesystem::path path;
        size_t size;
        chunk_id_t cur_chunk;
    };

    std::vector<queuedFile> queuedFiles;            //files that have already been queued to be sent and the destination has replied accepting the transfer
    std::vector<queuedFile> pendingFileHeaders;     //file headers that we sent and that are waiting for an ack
    std::vector<pendingRecvFile> pendingRecvFiles;  //files that we have accepted to recieve, and are waiting on chunks

    void handleFileHeader(const Data::File::FileHeader &message);
    void handleFileChunk(const Data::File::FileChunk &message);
    void handleFileChunkAck(const Data::File::FileChunkAck &message);
    void handleFileHeaderAck(const Data::File::FileHeaderAck &message);
};

}

#endif