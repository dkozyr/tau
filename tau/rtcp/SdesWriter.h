#pragma once

#include "tau/rtcp/Writer.h"
#include "tau/rtcp/Sdes.h"
#include "tau/common/Math.h"
#include "tau/common/Log.h"

namespace tau::rtcp {

class SdesWriter {
public:
    static bool Write(Writer& writer, const SdesChunks& chunks) {
        const auto length = kHeaderSize + GetChunksSize(chunks);
        if(writer.GetAvailableSize() < length) {
            return false;
        }

        writer.WriteHeader(Type::kSdes, chunks.size(), length);
        for(auto& chunk : chunks) {
            writer.Write(chunk.ssrc);
            for(auto& item : chunk.items) {
                writer.Write(static_cast<uint8_t>(item.type));
                writer.Write(static_cast<uint8_t>(item.data.size()));
                writer.Write(item.data);
            }
            writer.Write(static_cast<uint8_t>(SdesType::kEnd));
            const auto size = writer.GetSize();
            const auto padding = Align(size, sizeof(uint32_t)) - size;
            for(size_t i = 0; i < padding; ++i) {
                writer.Write(static_cast<uint8_t>(0));
            }
        }
        return true;
    }

private:
    static size_t GetChunksSize(const SdesChunks& chunks) {
        size_t size = 0;
        for(auto& chunk : chunks) {
            size += GetChunkSize(chunk);
        }
        return size;
    }

    static size_t GetChunkSize(const SdesChunk& chunk) {
        size_t chunk_size = sizeof(uint32_t);
        for(auto& item : chunk.items) {
            chunk_size += 2 * sizeof(uint8_t);
            chunk_size += item.data.size();
        }
        chunk_size += sizeof(uint8_t);
        return Align(chunk_size, sizeof(uint32_t));
    }
};

}
