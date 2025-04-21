#include "tests/lib/Common.h"

namespace tau::webrtc {

class PeerConnectionTest : public ::testing::Test {
public:
    PeerConnectionTest()
        : _io(std::thread::hardware_concurrency())
    {}

    ~PeerConnectionTest() {
        _io.Join();
    }

protected:

protected:
    ThreadPool _io;
    SteadyClock _clock;
    std::optional<sdp::Sdp> _sdp_offer;
    std::optional<sdp::Sdp> _sdp_answer;
    crypto::Certificate _dtls_cert;
    std::vector<net::UdpSocketPtr> _udp_sockets;
    std::optional<ice::Agent> _ice_agent;
    std::vector<std::string> _local_ice_candidates;
};

TEST_F(PeerConnectionTest, DISABLED_MANUAL_Localhost) {
    crypto::Certificate ca(crypto::Certificate::Options{
        .cert = std::string{PROJECT_SOURCE_DIR} + "/data/ca.crt",
        .key  = std::string{PROJECT_SOURCE_DIR} + "/data/ca.key"
    });
    crypto::Certificate cert(crypto::Certificate::OptionsSelfSigned{.ca = ca});

    http::Server server(
        http::Server::Dependencies{.executor = _io.GetExecutor()},
        http::Server::Options{
            .port = 8888,
            .ssl_ctx = CreateSslContextPtr(cert.GetCertificateBuffer(), cert.GetPrivateKeyBuffer())
        });

}

}
