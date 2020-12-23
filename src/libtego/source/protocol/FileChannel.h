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
    typedef quint32 FileId;
    typedef quint32 ChunkId;
    static const int FileMaxChunkSize = 2000;

    explicit FileChannel(Direction direction, Connection *connection);

    bool sendFileWithId(QString file_url, QDateTime time, FileId id);
    bool sendNextChunk(FileId id);
    bool sendChunkWithId(FileId fid, ChunkId cid);
    bool sendChunkWithId(FileId fid, std::filesystem::path &fpath, ChunkId cid);

signals:
protected:
    virtual bool allowInboundChannelRequest(const Data::Control::OpenChannel *request, Data::Control::ChannelResult *result);
    virtual bool allowOutboundChannelRequest(Data::Control::OpenChannel *request);
    virtual void receivePacket(const QByteArray &packet);
private:
    FileId nextFileId();
    FileId file_id;
    
    struct queuedFile {
        FileId id;
        std::filesystem::path path;
        size_t size;
        ChunkId last_chunk;
        bool peer_did_accept;
    };

    struct pendingRecvFile {
        FileId id;
        std::filesystem::path path;
        size_t size;
        ChunkId last_chunk;
    };

    std::vector<queuedFile> queuedFiles;            //files that have already been queued to be sent and the destination has replied accepting the transfer
    std::vector<queuedFile> pendingFileHeaders;     //file headers that are waiting for an ack
    std::vector<pendingRecvFile> pendingRecvFiles;  //files that we have accepted to recieve, and are waiting on chunks

    void handleFileHeader(const Data::File::FileHeader &message);
    void handleFileChunk(const Data::File::FileChunk &message);
    void handleFileChunkAck(const Data::File::FileChunkAck &message);
    void handleFileHeaderAck(const Data::File::FileHeaderAck &message);
};

}

#endif