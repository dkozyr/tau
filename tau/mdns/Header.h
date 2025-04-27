#pragma once

#include "tau/memory/Writer.h"
#include <cstdint>
#include <cstddef>

namespace tau::mdns {

inline constexpr size_t kHeaderSize = 6 * sizeof(uint16_t);
inline constexpr uint16_t kAnnouncement = 0x8400;
inline constexpr uint16_t kQuestion = 0;
inline constexpr uint16_t kInClass = 1;

enum class Type : uint16_t {
    kIpV4 = 1,  // A
    kIpV6 = 28, // AAAA
    kAll  = 255
};

class HeaderReader {
public:
    static uint16_t GetId(const BufferViewConst& view);
    static uint16_t GetFlags(const BufferViewConst& view);
    static uint16_t GetQuestionsCount(const BufferViewConst& view);
    static uint16_t GetAnswersCount(const BufferViewConst& view);
    static uint16_t GetAuthorityCount(const BufferViewConst& view);
    static uint16_t GetAdditionalCount(const BufferViewConst& view);

    static bool Validate(const BufferViewConst& view);
};

class HeaderWriter {
public:
    static bool Write(Writer& writer, uint16_t id, uint16_t flags, uint16_t questions, uint16_t answers,
                      uint16_t authority = 0, uint16_t additional = 0);
};

}
