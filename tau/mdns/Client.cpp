#include "tau/mdns/Client.h"
#include "tau/mdns/Header.h"
#include "tau/mdns/Question.h"
#include "tau/mdns/Answer.h"
#include "tau/common/NetToHost.h"
#include "tau/common/String.h"
#include "tau/common/Uuid.h"
#include "tau/common/Log.h"

namespace tau::mdns {

Client::Client(Dependencies&& deps)
    : _deps(std::move(deps))
{}

std::string Client::CreateName(IpAddressV4 address) {
    UpdateContexts();

    const auto name = GenerateUuid() + ".local";
    _name_to_ctx.insert({name, Context{
        .address = address,
        .tp_eol = _deps.clock.Now() + kNameTimeout,
    }});
    SendAnnouncement(0, name, address, kNameTimeoutSec); //TODO: re-send in 1 second?
    return name;
}

//TODO: keep list of active names to avoid sending question
void Client::FindIpAddressByName(const std::string& name, OnFoundIpAddressCallback callback) {
    _on_found_ip_address_callbacks.insert({name, QuestionContext{
        .callback = std::move(callback),
        .tp_eol = _deps.clock.Now() + 4 * kSec
    }});

    auto packet = Buffer::Create(_deps.udp_allocator);
    Writer writer(packet.GetViewWithCapacity());
    const auto id = static_cast<uint16_t>(_deps.clock.Now() & 0xFFFF);
    if(!HeaderWriter::Write(writer, id, kQuestion, 1, 0)) {
        return;
    }
    if(!QuestionWriter::Write(writer, name, Type::kIpV4, kInClass)) {
        return;
    }
    packet.SetSize(writer.GetSize());
    _send_callback(std::move(packet));
}

void Client::Recv(Buffer&& packet) {
    UpdateContexts();

    auto view = ToConst(packet.GetView());
    if(!HeaderReader::Validate(view)) {
        return;
    }

    auto questions_count = HeaderReader::GetQuestionsCount(view);
    auto answers_count = HeaderReader::GetAnswersCount(view);
    BufferViewConst payload{.ptr = view.ptr + kHeaderSize, .size = view.size - kHeaderSize};
    for(size_t i = 0; i < questions_count; ++i) {
        auto question = ParseQuestion(payload);
        if(!question) {
            return;
        }
        OnQuestion(HeaderReader::GetId(view), *question);
        payload.ptr += question->size;
        payload.size -= question->size;
    }

    for(size_t i = 0; i < answers_count; ++i) {
        auto answer = ParseAnswer(payload);
        if(!answer) {
            return;
        }
        OnAnswer(*answer);
        payload.ptr = answer->data.ptr + answer->data.size;
        payload.size = view.ptr + view.size - payload.ptr;
    }
}

void Client::OnQuestion(uint16_t id, const Question& question) {
    TAU_LOG_DEBUG("Name: " << question.name << ", type: " << question.type << ", class: " << question.cash_flush_and_class);
    auto it = _name_to_ctx.find(question.name);
    if(it != _name_to_ctx.end()) {
        const auto& ctx = it->second;
        const auto ttl = (ctx.tp_eol - _deps.clock.Now()) / kSec;
        SendAnnouncement(id, question.name, ctx.address, ttl);
    }
}

void Client::OnAnswer(const Answer& answer) {
    TAU_LOG_DEBUG("Name: " << answer.name << ", type: " << answer.type << ", class: " << answer.cash_flush_and_class << ", ttl: " << answer.ttl);
    auto it = _on_found_ip_address_callbacks.find(answer.name);
    if(it != _on_found_ip_address_callbacks.end()) {
        if(answer.data.size == sizeof(uint32_t)) {
            const auto value = Read32(answer.data.ptr);
            TAU_LOG_DEBUG("Found name: " << answer.name << ", ip: " << IpAddressV4(value));
            auto& ctx = it->second;
            ctx.callback(IpAddressV4(value));
        }
        _on_found_ip_address_callbacks.erase(it);
    }
}

void Client::UpdateContexts() {
    const auto now = _deps.clock.Now();
    for(auto it = _name_to_ctx.begin(); it != _name_to_ctx.end(); ) {
        const auto& ctx = it->second;
        if(now >= ctx.tp_eol) {
            SendAnnouncement(0, it->first, ctx.address, 0);
            it = _name_to_ctx.erase(it);
        } else {
            it++;
        }
    }

    for(auto it = _on_found_ip_address_callbacks.begin(); it != _on_found_ip_address_callbacks.end(); ) {
        const auto& ctx = it->second;
        if(now >= ctx.tp_eol) {
            it = _on_found_ip_address_callbacks.erase(it);
        } else {
            it++;
        }
    }
}

void Client::SendAnnouncement(uint16_t id, std::string_view name, IpAddressV4 address, uint32_t ttl) {
    auto packet = Buffer::Create(_deps.udp_allocator);
    Writer writer(packet.GetViewWithCapacity());
    if(!HeaderWriter::Write(writer, id, kAnnouncement, 0, 1)) {
        return;
    }
    if(!AnswerWriter::Write(writer, name, Type::kIpV4, kInClass, ttl, address.to_uint())) {
        return;
    }
    packet.SetSize(writer.GetSize());
    _send_callback(std::move(packet));
}

}
