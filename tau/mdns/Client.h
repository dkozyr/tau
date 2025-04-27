#pragma once

#include "tau/mdns/Question.h"
#include "tau/mdns/Answer.h"
#include "tau/memory/Buffer.h"
#include "tau/asio/Common.h"
#include "tau/common/Clock.h"
#include <unordered_map>

namespace tau::mdns {

class Client {
public:
    static constexpr Timepoint kNameTimeoutSec = 120;
    static constexpr Timepoint kNameTimeout = kNameTimeoutSec * kSec;

    struct Dependencies {
        Allocator& udp_allocator;
        Clock& clock;
    };

    using SendCallback = std::function<void(Buffer&& packet)>;
    using OnFoundIpAddressCallback = std::function<void(IpAddressV4 address)>;

public:
    Client(Dependencies&& deps);

    void SetSendCallback(SendCallback callback) { _send_callback = std::move(callback); }

    std::string CreateName(IpAddressV4 address);
    void FindIpAddressByName(const std::string& name, OnFoundIpAddressCallback callback);
    void Recv(Buffer&& packet);

private:
    void OnQuestion(uint16_t id, const Question& question);
    void OnAnswer(const Answer& answer);
    void UpdateContexts();

    void SendAnnouncement(uint16_t id, std::string_view name, IpAddressV4 address, uint32_t ttl);

private:
    Dependencies _deps;
    SendCallback _send_callback;

    struct Context {
        IpAddressV4 address;
        Timepoint tp_eol;
    };
    std::unordered_map<std::string, Context> _name_to_ctx;

    struct QuestionContext {
        OnFoundIpAddressCallback callback;
        Timepoint tp_eol;
    };
    std::unordered_map<std::string, QuestionContext> _on_found_ip_address_callbacks;
};

}
