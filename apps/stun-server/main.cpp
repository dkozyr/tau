#include "tau/net/UdpSocket.h"
#include "tau/stun/Reader.h"
#include "tau/stun/Writer.h"
#include "tau/stun/attribute/XorMappedAddress.h"
#include "tau/stun/attribute/ByteString.h"
#include "tau/stun/attribute/Fingerprint.h"
#include "tau/asio/ThreadPool.h"
#include "tau/asio/PeriodicTimer.h"
#include "tau/memory/PoolAllocator.h"
#include "tau/common/Clock.h"
#include "tau/common/Log.h"
#include <atomic>

using namespace tau;

int main(int /*argc*/, char** /*argv*/) {
    InitLogging("tau-stun-server", false);

    SteadyClock clock;
    ThreadPool io(std::thread::hardware_concurrency());

    std::atomic<size_t> requests{0};
    std::atomic<size_t> responses{0};

    PoolAllocator udp_allocator(1200);
    auto udp_socket = net::UdpSocket::Create(
        net::UdpSocket::Options{
            .allocator = udp_allocator,
            .executor = io.GetExecutor(),
            .local_address = "0.0.0.0",
            .local_port = 3478
        });
    udp_socket->SetRecvCallback([&](Buffer&& message, Endpoint remote_endpoint) {
        requests.fetch_add(1);
        auto view_const = ToConst(message.GetView());
        if(!stun::Reader::Validate(view_const)) {
            TAU_LOG_WARNING_THR(128, "Invalid STUN message, remote_endpoint: " << remote_endpoint);
            return;
        }
        if(stun::kBindingRequest != stun::HeaderReader::GetType(view_const)) {
            TAU_LOG_WARNING_THR(128, "STUN message wrong type, remote_endpoint: " << remote_endpoint);
            return;
        }

        responses.fetch_add(1);
        stun::Writer writer(message.GetViewWithCapacity(), stun::kBindingResponse);
        stun::attribute::XorMappedAddressWriter::Write(writer,
            stun::AttributeType::kXorMappedAddress,
            remote_endpoint.address().to_v4().to_uint(),
            remote_endpoint.port());
        stun::attribute::ByteStringWriter::Write(writer,
            stun::AttributeType::kSoftware,
            "TAU WebRTC Library");
        stun::attribute::FingerprintWriter::Write(writer);
        message.SetSize(writer.GetSize());

        udp_socket->Send(std::move(message), remote_endpoint);
    });
    udp_socket->SetErrorCallback([](boost_ec ec) {
        TAU_LOG_WARNING_THR(128, "[udp socket] Error: " << ec.value() << ", " << ec.message());
    });
    TAU_LOG_INFO("Listening: " << udp_socket->GetLocalEndpoint());

    PeriodicTimer timer(io.GetExecutor());
    constexpr auto kPrintStatsPeriodMs = 5 * 60 * 1000;
    timer.Start(kPrintStatsPeriodMs, [&](beast_ec ec) {
        if(ec) {
            TAU_LOG_WARNING("Error: " << ec.value() << ", " << ec.message());
            return false;
        }
        TAU_LOG_INFO("[stats] Requests: " << requests << ", responses: " << responses);
        return true;
    });

    io.Join();
    return 0;
}
