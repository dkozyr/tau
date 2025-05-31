#include "tau/mdns/Question.h"
#include "tau/mdns/NameReader.h"
#include "tau/common/NetToHost.h"
#include "tau/common/String.h"

namespace tau::mdns {

std::optional<Question> ParseQuestion(const BufferViewConst& view) {
    auto ptr = view.ptr;
    auto end = view.ptr + view.size;

    const auto name = ParseName(ptr, end);
    if(name.empty()) {
        return std::nullopt;
    }

    auto payload_end = ptr + 2 * sizeof(uint16_t);
    if(payload_end > end) {
        return std::nullopt;
    }

    return Question{
        .name = name,
        .type = Read16(ptr),
        .cash_flush_and_class = Read16(ptr + sizeof(uint16_t)),
        .size = static_cast<size_t>(payload_end - view.ptr)
    };
}

bool QuestionWriter::Write(Writer& writer, std::string_view name, Type type, uint16_t class_) {
    auto labels = Split(name, ".");
    const auto name_size = name.size() + 1 + 1;
    if(writer.GetAvailableSize() < name_size + 2 * sizeof(uint16_t)) {
        return false;
    }
    for(auto& label : labels) {
        writer.Write(static_cast<uint8_t>(label.size()));
        writer.Write(label);
    }
    writer.Write(static_cast<uint8_t>(0));
    writer.Write(static_cast<uint16_t>(type));
    writer.Write(class_);
    return true;
}

}
