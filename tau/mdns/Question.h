#pragma once

#include "tau/mdns/Header.h"
#include "tau/memory/Writer.h"
#include <string>
#include <optional>

namespace tau::mdns {

struct Question {
    std::string name;
    uint16_t type;
    uint16_t cash_flush_and_class;
    size_t size;
};

std::optional<Question> ParseQuestion(const BufferViewConst& view);

class QuestionWriter {
public:
    static bool Write(Writer& writer, std::string_view name, Type type, uint16_t class_);
};

}
