#pragma once

#include "tau/mdns/Header.h"
#include "tau/mdns/Name.h"
#include "tau/memory/Writer.h"
#include <etl/string.h>
#include <optional>

namespace tau::mdns {

struct Answer {
    Name name;
    uint16_t type;
    uint16_t cash_flush_and_class;
    uint32_t ttl;
    BufferViewConst data;
};

std::optional<Answer> ParseAnswer(const BufferViewConst& view);

class AnswerWriter {
public:
    static bool Write(Writer& writer, const etl::string_view& name, Type type, uint16_t class_, uint32_t ttl, uint32_t ip_address_v4);
};

}
