#pragma once

#include "tau/rtcp/Header.h"
#include "tau/rtcp/Sdes.h"
#include "tau/common/NetToHost.h"
#include "tau/common/Math.h"
#include <optional>
#include <cassert>

namespace tau::rtcp {

class SdesReader {
public:
    static SdesChunks GetChunks(const BufferViewConst& view) {
        const auto chunks_count = GetRc(view.ptr[0]);
        SdesChunks chunks;
        chunks.reserve(chunks_count);
        auto ptr = view.ptr + kHeaderSize;
        for(size_t i = 0; i < chunks_count; ++i) {
            chunks.push_back(SdesChunk{
                .ssrc = Read32(ptr),
                .items = ParseItems(ptr + sizeof(uint32_t))
            });
            ptr += sizeof(uint32_t) + GetItemsSize(view, ptr + sizeof(uint32_t) - view.ptr).value();
        }
        return chunks;
    }

    static bool Validate(const BufferViewConst& view) {
        assert(view.size >= kHeaderSize); // validated by rtcp::Reader
        const auto chunks_count = GetRc(view.ptr[0]);
        size_t length = kHeaderSize;
        for(size_t i = 0; i < chunks_count; ++i) {
            length += sizeof(uint32_t);
            auto items_size = GetItemsSize(view, length);
            if(items_size) {
                length += *items_size;
            } else {
                return false;
            }
        }
        return (view.size == length);
    }

private:
    static SdesItems ParseItems(const uint8_t* ptr) {
        SdesItems items;
        while(true) {
            const auto type = static_cast<SdesType>(ptr[0]);
            if(type == SdesType::kEnd) {
                break;
            }
            items.push_back(SdesItem{
                .type = type,
                .data = std::string_view{reinterpret_cast<const char*>(ptr) + 2, ptr[1]}
            });
            ptr += 2 * sizeof(uint8_t) + items.back().data.size();
        }
        return items;
    }

    static std::optional<size_t> GetItemsSize(const BufferViewConst& view, size_t offset) {
        size_t items_size = 0;
        while(true) {
            if(offset >= view.size) {
                return std::nullopt;
            }
            auto type = static_cast<SdesType>(view.ptr[offset]);
            if(type == SdesType::kEnd) {
                items_size++;
                break;
            }
            offset++;
            if(offset >= view.size) {
                return std::nullopt;
            }
            auto size = view.ptr[offset];
            offset++;
            if(offset + size >= view.size) {
                return std::nullopt;
            }
            offset += size;
            items_size += 2 * sizeof(uint8_t) + size;
        }
        return Align(items_size, sizeof(uint32_t));
    }
};

}
