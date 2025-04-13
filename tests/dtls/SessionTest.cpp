#include "tau/dtls/Session.h"
#include "tests/lib/Common.h"

namespace tau::dtls {

TEST(SessionTest, Basic) {
    TestClock clock;

    Session server(
        Session::Dependencies{.clock = clock, .udp_allocator = g_udp_allocator},
        Session::Options{
            .type = Session::Type::kServer,
            .remote_peer_cert_digest = "hello",
            .log_ctx = "[server] "
        });
    Session client(
        Session::Dependencies{.clock = clock, .udp_allocator = g_udp_allocator},
        Session::Options{
            .type = Session::Type::kClient,
            .remote_peer_cert_digest = "worlds",
            .log_ctx = "[client] "
        });

    std::queue<std::pair<bool, Buffer>> queue;
    server.SetSendCallback([&](Buffer&& packet) { queue.push(std::make_pair(true, std::move(packet))); });
    client.SetSendCallback([&](Buffer&& packet) { queue.push(std::make_pair(false, std::move(packet))); });

    server.SetRecvCallback([&](Buffer&& packet) {
        auto view = packet.GetView();
        auto message = std::string_view{reinterpret_cast<char*>(view.ptr), view.size};
        TAU_LOG_INFO("[server] [recv] message: " << message);
    });
    client.SetRecvCallback([&](Buffer&& packet) {
        auto view = packet.GetView();
        auto message = std::string_view{reinterpret_cast<char*>(view.ptr), view.size};
        TAU_LOG_INFO("[client] [recv] message: " << message);
    });

    server.SetStateCallback([&](Session::State state) { TAU_LOG_INFO("[server] state: " << (size_t)state); });
    client.SetStateCallback([&](Session::State state) { TAU_LOG_INFO("[client] state: " << (size_t)state); });

    auto process_queue = [&]() {
        for(auto i = 0; i < 4; ++i) {
            server.Process();
            client.Process();
            while(!queue.empty()) {
                // TAU_LOG_INFO("queue: " << queue.size());   
                auto& [from_server, packet] = queue.front();
                if(from_server) {
                    client.Recv(std::move(packet));
                } else {
                    server.Recv(std::move(packet));
                }
                queue.pop();
            }
        }
    };

    client.Process();
    process_queue();

    TAU_LOG_INFO("DTLS connection is established?");

    const char* message = "Hello from DTLS client!";
    ASSERT_TRUE(client.Send(BufferViewConst{.ptr = (const uint8_t*)message, .size = strlen(message)}));
    // client.Send(BufferViewConst{.ptr = (const uint8_t*)message, .size = strlen(message)});

    process_queue();

    TAU_LOG_INFO("Closing DTLS connection...");
    server.Stop();
    client.Stop();

    process_queue();
}

}
