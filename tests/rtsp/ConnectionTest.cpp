#include "tau/rtsp/Connection.h"
#include "tau/asio/ThreadPool.h"
#include "tau/rtsp/ResponseReader.h"
#include <gtest/gtest.h>
#include "tests/rtsp-test-lib/Server.h"
#include <atomic>
#include <algorithm>
#include <thread>

namespace tau::rtsp {

using namespace std::chrono_literals;

class ConnectionTest : public ::testing::Test {
protected:
    ConnectionTest()
        : _pool(std::max(1u, std::thread::hardware_concurrency()))
    {}

public:
    static StreamReader::String Request(const char* cseq) {
        StreamReader::String request{"OPTIONS rtsp://127.0.0.1/ RTSP/1.0\r\nCSeq: "};
        request.append(cseq);
        request.append("\r\n\r\n");
        return request;
    }

    static etl::string<128> Response(etl::string_view cseq) {
        etl::string<128> response{"RTSP/1.0 200 OK\r\nCSeq: "};
        constexpr etl::string_view kSuffix{"\r\nContent-Length: 0\r\n\r\n"};
        if(cseq.size() + kSuffix.size() > response.available()) { throw std::length_error("Response exceeds capacity"); }
        response.append(cseq);
        response.append(kSuffix);
        return response;
    }

protected:
    ThreadPool _pool;
};

TEST_F(ConnectionTest, ReusesConnectionAndReceivesPacketsWhileIdle) {
    std::atomic<size_t> requests{0};
    Server server([&](Server& server, StreamReader::Message&&) {
        const auto count = ++requests;
        const auto response = Response(count == 1 ? "1" : "2");
        server.SendBuffer({response.data(), 13});

        etl::string<256> remainder{response.data() + 13, response.size() - 13};
        ASSERT_GE(remainder.available(), size_t{7});

        remainder.append("$\7\0\3abc", 7);
        server.SendBuffer(remainder);
    });

    Connection connection(_pool.GetExecutor(),
        Connection::Options{
            .host = "127.0.0.1",
            .port = server.Port(),
            .timeout = 1s
        });

    std::promise<void> packets_received;
    auto packets_ready = packets_received.get_future();

    size_t packets = 0;
    connection.SetPacketCallback([&](StreamReader::Message&& message) {
        EXPECT_EQ(7, message.channel);
        EXPECT_EQ("abc", message.data);
        if(++packets == 2) {
            packets_received.set_value();
        }
    });

    auto first = connection.SendRequest(Request("1"), "1");
    ASSERT_EQ(std::future_status::ready, first.wait_for(2s));

    auto first_message = first.get();
    auto first_response = ResponseReader::Read(first_message.data);
    ASSERT_TRUE(first_response);

    auto second = connection.SendRequest(Request("2"), "2");
    ASSERT_EQ(std::future_status::ready, second.wait_for(2s));
    EXPECT_NO_THROW(second.get());
    ASSERT_EQ(std::future_status::ready, packets_ready.wait_for(2s));
    EXPECT_EQ(2, requests);
    EXPECT_EQ("1", GetHeaderValue(HeaderName::kCSeq, first_response->headers));

    connection.Close();
}

TEST_F(ConnectionTest, MatchesOutOfOrderResponsesAndSerializesPacketWrites) {
    std::promise<void> packet_received;
    auto packet_ready = packet_received.get_future();

    size_t requests = 0;
    Server server([&](Server& server, StreamReader::Message&& message) {
        if(message.channel) {
            EXPECT_EQ(5, message.channel);
            EXPECT_EQ("report", message.data);
            packet_received.set_value();
        } else if(++requests == 2) {
            etl::string<256> responses{Response("2")};
            const auto first = Response("1");
            ASSERT_LE(first.size(), responses.available());
            responses.append(first);
            server.SendBuffer(responses);
        } else if(requests == 3) {
            server.SendBuffer(Response("3"));
        }
    });

    Connection connection(_pool.GetExecutor(),
        Connection::Options{
            .host = "127.0.0.1",
            .port = server.Port(),
            .timeout = 1s
        });

    auto first = connection.SendRequest(Request("1"), "1");
    auto second = connection.SendRequest(Request("2"), "2");
    ASSERT_EQ(std::future_status::ready, first.wait_for(2s));
    ASSERT_EQ(std::future_status::ready, second.wait_for(2s));
    EXPECT_NO_THROW(first.get());
    EXPECT_NO_THROW(second.get());

    auto third = connection.SendRequest(Request("3"), "3");
    auto sent = connection.SendPacket(5, StreamReader::String{"report"});
    ASSERT_EQ(std::future_status::ready, sent.wait_for(2s));
    EXPECT_NO_THROW(sent.get());
    ASSERT_EQ(std::future_status::ready, third.wait_for(2s));
    EXPECT_NO_THROW(third.get());
    EXPECT_EQ(std::future_status::ready, packet_ready.wait_for(2s));

    connection.Close();
}

TEST_F(ConnectionTest, TimeoutAndCancellation) {
    Server server([](Server&, StreamReader::Message&&) {});
    {
        Connection connection(_pool.GetExecutor(),
            Connection::Options{
                .host = "127.0.0.1",
                .port = server.Port(),
                .timeout = 50ms
            });
        auto response = connection.SendRequest(Request("1"), "1");
        ASSERT_EQ(std::future_status::ready, response.wait_for(2s));
        EXPECT_THROW(response.get(), std::runtime_error);
    }
    std::future<StreamReader::Message> cancelled;
    {
        Connection connection(_pool.GetExecutor(),
            Connection::Options{
                .host = "127.0.0.1",
                .port = server.Port(),
                .timeout = 1s
            });
        cancelled = connection.SendRequest(Request("2"), "2");
    }
    ASSERT_EQ(std::future_status::ready, cancelled.wait_for(2s));
    EXPECT_THROW(cancelled.get(), std::runtime_error);
}

TEST_F(ConnectionTest, RejectsMismatchedAndTruncatedResponses) {
    for(const auto mismatch : {false, true}) {
        Server server([&](Server& server, StreamReader::Message&&) {
            if(mismatch) { server.SendBuffer(Response("99")); }
            else { server.SendBuffer("RTSP/1.0 200 OK\r\nCSeq: 1\r\nContent-Length: 8\r\n\r\nabc"); }
            if(!mismatch) {
                server.Disconnect();
            }
        });

        Connection connection(_pool.GetExecutor(),
            Connection::Options{
                .host = "127.0.0.1",
                .port = server.Port(),
                .timeout = 1s
            });

        auto response = connection.SendRequest(Request("1"), "1");
        ASSERT_EQ(std::future_status::ready, response.wait_for(2s));
        EXPECT_THROW(response.get(), std::runtime_error);

        connection.Close();
    }
}

TEST_F(ConnectionTest, RejectsOversizedCSeq) {
    Connection connection(_pool.GetExecutor(),
        Connection::Options{
            .host = "127.0.0.1"
        });

    auto response = connection.SendRequest(Request("12345678901234567"), "12345678901234567");
    ASSERT_EQ(std::future_status::ready, response.wait_for(1s));
    EXPECT_THROW(response.get(), std::invalid_argument);

    connection.Close();
}

TEST_F(ConnectionTest, CloseInsidePacketCallbackRetainsCaptures) {
    Server server([](Server& server, StreamReader::Message&&) {
        server.SendBuffer(etl::string_view{"$\1\0\1x", 5});
    });
    Connection connection(_pool.GetExecutor(),
        Connection::Options{
            .host = "127.0.0.1",
            .port = server.Port(),
            .timeout = 1s
        });

    auto token = std::make_shared<int>(42);
    const auto weak_token = std::weak_ptr<int>(token);
    connection.SetPacketCallback([&, token = std::move(token)](StreamReader::Message&&) {
        connection.Close();
        EXPECT_FALSE(weak_token.expired());
        EXPECT_EQ(42, *token);
    });

    auto response = connection.SendRequest(Request("1"), "1");
    ASSERT_EQ(std::future_status::ready, response.wait_for(2s));
    EXPECT_THROW(response.get(), std::runtime_error);

    _pool.Join();
    EXPECT_TRUE(weak_token.expired());
}

TEST_F(ConnectionTest, AsyncCloseDetachesCallbacksBeforeWrapperDestruction) {
    std::promise<void> packet_entered;
    std::promise<void> release_packet;
    std::promise<void> input_sent;

    auto entered = packet_entered.get_future();
    auto released = release_packet.get_future();
    auto sent = input_sent.get_future();

    Server server([&](Server& server, StreamReader::Message&&) {
        server.SendBuffer(etl::string_view{"$\1\0\1x", 5});
        server.SendBuffer(etl::string_view{"$\1\0\1y$\1\0\1z", 10});
        input_sent.set_value();
    });

    bool completed = false;
    size_t packets = 0;
    size_t packets_at_close = 0;
    size_t closes = 0;

    auto token = std::make_shared<int>(42);
    const auto weak_token = std::weak_ptr<int>(token);

    std::optional<Connection> connection;
    connection.emplace(_pool.GetExecutor(),
        Connection::Options{
            .host = "127.0.0.1",
            .port = server.Port(),
            .timeout = 5s
        });
    connection->SetPacketCallback([&, token](StreamReader::Message&&) {
        EXPECT_FALSE(completed);
        EXPECT_EQ(*token, 42);
        if(++packets == 1) {
            packet_entered.set_value();
            EXPECT_EQ(released.wait_for(2s), std::future_status::ready);
        }
    });
    connection->SetCloseCallback([&, token]() {
        EXPECT_FALSE(completed);
        EXPECT_EQ(*token, 42);
        ++closes;
    });
    token.reset();

    auto response = connection->SendRequest(Request("1"), "1");
    EXPECT_EQ(entered.wait_for(2s), std::future_status::ready);
    EXPECT_EQ(sent.wait_for(2s), std::future_status::ready);

    std::promise<void> close_completed;
    auto closed = close_completed.get_future();
    connection->AsyncClose([&]() {
        completed = true;
        packets_at_close = packets;
        EXPECT_TRUE(weak_token.expired());

        connection.reset();
        close_completed.set_value();
    });
    release_packet.set_value();

    EXPECT_EQ(closed.wait_for(2s), std::future_status::ready);
    _pool.Join();

    EXPECT_FALSE(connection.has_value());
    EXPECT_EQ(packets, packets_at_close);
    EXPECT_GE(packets, 1);
    EXPECT_EQ(closes, 1);
    ASSERT_EQ(response.wait_for(0s), std::future_status::ready);
    EXPECT_THROW(response.get(), std::runtime_error);
}

}
