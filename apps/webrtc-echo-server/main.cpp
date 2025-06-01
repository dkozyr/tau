#include "apps/webrtc-echo-server/Session.h"
#include "apps/webrtc-echo-server/Config.h"
#include "tau/ws/Server.h"
#include "tau/srtp/Common.h"
#include "tau/crypto/Certificate.h"
#include "tau/asio/ThreadPool.h"
#include "tau/memory/PoolAllocator.h"
#include "tau/net/Uri.h"
#include "tau/common/Clock.h"
#include "tau/common/Log.h"
#include <boost/program_options.hpp>
#include <atomic>
#include <mutex>

using namespace tau;

SslContextPtr CreateSslContextInternal(const Config::Ssl& config_ssl);

int main(int argc, char** argv) {
    namespace po = boost::program_options;

    std::string config_path = std::string{PROJECT_SOURCE_DIR} + "/data/webrtc-echo-server-config.json";
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("config", po::value<std::string>(&config_path)->default_value(config_path), "Config file path")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if(vm.count("help")) {
        TAU_LOG_INFO(desc);
        return 1;
    }

    auto config = ParseAndValidateConfig(config_path);
    if(!config) {
        return -1;
    }

    InitLogging("tau-webrtc-echo-server", config->logging.console, config->logging.severity);
    srtp::Init();

    SteadyClock clock;
    PoolAllocator udp_allocator(1500);
    ThreadPool io(std::thread::hardware_concurrency());

    SslContextPtr ssl_ctx = CreateSslContextInternal(config->ssl);

    ws::Server server(
        ws::Server::Dependencies{.executor = io.GetExecutor()},
        ws::Server::Options{config->ip.private_ip, config->wss.port, *ssl_ctx, config->wss.http_fields}
    );
    if(!config->wss.validation.origin_host.empty()) {
        server.SetValidateRequestCallback(
            [expected_origin_host = config->wss.validation.origin_host](const beast_request& request) {
                // TAU_LOG_INFO("Request: " << request);
                const auto it_origin = request.find("Origin");
                if(it_origin == request.end()) {
                    TAU_LOG_WARNING("No Origin header");
                    return false;
                }
                auto uri = net::GetUriFromString(it_origin->value());
                if(!uri || (uri->host != expected_origin_host)) {
                    TAU_LOG_WARNING("Wrong Origin header: " << it_origin->value());
                    return false;
                }
                return true;
            });
    }

    std::mutex mutex;
    std::list<SessionPtr> sessions;
    std::atomic<size_t> connections{0};

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
    auto print_stats_tp = clock.Now();
    timer.Start(kPrintStatsPeriodMs, [&](beast_ec ec) {
        if(ec) {
            TAU_LOG_WARNING("Error: " << ec.value() << ", " << ec.message());
            return false;
        }
        std::lock_guard lock{mutex};
        bool print_stats = false;
        for(auto it = sessions.begin(); it != sessions.end();) {
            auto& session = *it;
            if(session->IsActive()) {
                it++;
            } else {
                it = sessions.erase(it);
                print_stats = true;
            }
        }
        const auto now = clock.Now();
        if(print_stats || (print_stats_tp + 10 * kMin < now)) {
            print_stats_tp = now;
            TAU_LOG_INFO("Connections: " << connections << ", active sessions: " << sessions.size());
        }
        return true;
    });

    io.Join();
    return 0;
}

SslContextPtr CreateSslContextInternal(const Config::Ssl& config_ssl) {
    if(config_ssl.self_signed) {
        crypto::Certificate ca(crypto::Certificate::Options{
            .cert = config_ssl.ca.certificate,
            .key  = config_ssl.ca.key
        });
        crypto::Certificate cert(crypto::Certificate::OptionsSelfSigned{.ca = ca});
        return CreateSslContextPtr(cert.GetCertificateBuffer(), cert.GetPrivateKeyBuffer());
    } else {
        crypto::Certificate cert(crypto::Certificate::Options{
            .cert = config_ssl.server.certificate,
            .key  = config_ssl.server.key
        });
        return CreateSslContextPtr(cert.GetCertificateBuffer(), cert.GetPrivateKeyBuffer());
    }
}
