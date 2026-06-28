#pragma once

#include "tau/mdns/Question.h"
#include "tau/mdns/Answer.h"
#include "tau/memory/Buffer.h"
#include "tau/net/IpAddress.h"
#include "tau/common/Clock.h"
#include <etl/unordered_map.h>

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
    using OnFoundIpAddressCallback = std::function<void(IpAddress address)>;

public:
    Client(Dependencies&& deps);

    void SetSendCallback(SendCallback callback) { _send_callback = std::move(callback); }

    Name CreateName(IpAddress address);
    void FindIpAddressByName(const etl::string_view& name, OnFoundIpAddressCallback callback);
    void Recv(Buffer&& packet);

private:
    void OnQuestion(uint16_t id, const Question& question);
    void OnAnswer(const Answer& answer);
    void UpdateContexts();

    void SendAnnouncement(uint16_t id, const etl::string_view& name, IpAddress address, uint32_t ttl);

private:
    Dependencies _deps;
    SendCallback _send_callback;

    struct Context {
        IpAddress address;
        Timepoint tp_eol;
    };
    etl::unordered_map<Name, Context, 16> _name_to_ctx;

    struct QuestionContext {
        OnFoundIpAddressCallback callback;
        Timepoint tp_eol;
    };
    etl::unordered_map<Name, QuestionContext, 16> _on_found_ip_address_callbacks;
};

}
