#include "tau/mdns/Answer.h"
#include "tau/mdns/NameReader.h"
#include "tau/common/NetToHost.h"
#include "tau/common/String.h"

namespace tau::mdns {

std::optional<Answer> ParseAnswer(const BufferViewConst& view) {
    auto ptr = view.ptr;
    auto end = view.ptr + view.size;

    const auto name = ParseName(ptr, end);
    if(name.empty()) {
        return std::nullopt;
    }

    auto data_ptr = ptr + 2 * sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t);
    if(data_ptr > end) {
        return std::nullopt;
    }

    auto data_size = Read16(ptr + 2 * sizeof(uint16_t) + sizeof(uint32_t));
    if(data_ptr + data_size > end) {
        return std::nullopt;
    }

    return Answer{
        .name = name,
        .type = Read16(ptr),
        .cash_flush_and_class = Read16(ptr + sizeof(uint16_t)),
        .ttl = Read32(ptr + 2 * sizeof(uint16_t)),
        .data = BufferViewConst{.ptr = data_ptr, .size = data_size}
    };
}

bool AnswerWriter::Write(Writer& writer, std::string_view name, Type type, uint16_t class_, uint32_t ttl, uint32_t ip_address_v4) {
    auto labels = Split(name, ".");
    const auto name_size = name.size() + 1 + 1;
    if(writer.GetAvailableSize() < name_size + 2 * sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint32_t)) {
        return false;
    }
    for(auto& label : labels) {
        writer.Write(static_cast<uint8_t>(label.size()));
        writer.Write(label);
    }
    writer.Write(static_cast<uint8_t>(0));
    writer.Write(static_cast<uint16_t>(type));
    writer.Write(class_);
    writer.Write(ttl);
    writer.Write(static_cast<uint16_t>(sizeof(ip_address_v4)));
    writer.Write(ip_address_v4);
    return true;
}

}
