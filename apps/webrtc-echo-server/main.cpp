#include "apps/webrtc-echo-server/Session.h"
#include "tau/ws/Server.h"
#include "tau/srtp/Common.h"
#include "tau/crypto/Certificate.h"
#include "tau/asio/ThreadPool.h"
#include "tau/memory/PoolAllocator.h"
#include "tau/common/Clock.h"
#include "tau/common/Log.h"
#include <atomic>
#include <mutex>

using namespace tau;

int main(int /*argc*/, char** /*argv*/) {
    InitLogging("tau-webrtc-echo-server", false);
    srtp::Init();

    SteadyClock clock;
    PoolAllocator udp_allocator(1500);
    ThreadPool io(std::thread::hardware_concurrency());

    std::atomic<size_t> connections{0};

    crypto::Certificate ca(crypto::Certificate::Options{
        .cert = std::string{PROJECT_SOURCE_DIR} + "/data/ca.crt",
        .key  = std::string{PROJECT_SOURCE_DIR} + "/data/ca.key"
    });
    crypto::Certificate cert(crypto::Certificate::OptionsSelfSigned{.ca = ca});
    SslContextPtr ssl_ctx(CreateSslContextPtr(cert.GetCertificateBuffer(), cert.GetPrivateKeyBuffer()));

    ws::Server server(
        ws::Server::Dependencies{.executor = io.GetExecutor()},
        ws::Server::Options{"127.0.0.1", 8443, *ssl_ctx}
    );

    std::mutex mutex;
    std::list<SessionPtr> sessions;

    server.SetOnNewConnectionCallback([&](ws::ConnectionPtr connection) {
        connections.fetch_add(1);
        std::lock_guard lock{mutex};
        sessions.push_back(std::make_unique<Session>(
            Session::Dependencies{
                .clock = clock,
                .executor = io.GetStrand(),
                .udp_allocator = udp_allocator
            }, std::move(connection)));
    });
    server.Start();

    PeriodicTimer timer(io.GetExecutor());
    constexpr auto kPrintStatsPeriodMs = 10 * 1000;
    timer.Start(kPrintStatsPeriodMs, [&](beast_ec ec) {
        if(ec) {
            TAU_LOG_WARNING("Error: " << ec.value() << ", " << ec.message());
            return false;
        }
        std::lock_guard lock{mutex};
        for(auto it = sessions.begin(); it != sessions.end();) {
            auto& session = *it;
            if(session->IsActive()) {
                it++;
            } else {
                it = sessions.erase(it);
            }
        }
        TAU_LOG_INFO("Connections: " << connections << ", active sessions: " << sessions.size());
        return true;
    });

    io.Join();
    return 0;
}
