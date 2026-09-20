#include "apps/rtsp-client/Maintenance.h"
#include "apps/rtsp-client/Utils.h"
#include "tests/rtsp-test-lib/Server.h"
#include "tests/apps/rtsp-client/MaintenanceClock.h"
#include "tau/asio/ThreadPool.h"
#include <gtest/gtest.h>
#include <thread>

namespace tau::rtsp {

using namespace std::chrono_literals;

class RtspMaintenanceTest : public testing::Test {
protected:
    ~RtspMaintenanceTest() override {
        if(_maintenance) { _maintenance->Stop(); }
        if(_connection) { _connection->Close(); }
        if(_session) { _session->Stop(); }
        _pool.Join();
        _server.reset();
    }

    void CreateMaintenance(size_t initial_cseq = 0) {
        _cseq = initial_cseq;
        _connection = std::make_shared<Connection>(
            _pool.GetExecutor(),
            Connection::Options{
                .host = "127.0.0.1",
                .port = _server->Port(),
                .timeout = 1s
            });
        _session = Session::Create(_pool.GetExecutor(), {}, Transport::kTcp);
        _maintenance.emplace(_clock, *_connection, _cseq);
    }

    void StartServer(etl::string_view status = "200 OK") {
        ASSERT_GE(32, status.size());

        const etl::string<32> response_status{status};
        _server = std::make_unique<Server>([this, response_status](Server& server, StreamReader::Message&& message) {
            EXPECT_EQ(0, message.data.find("OPTIONS "));

            const auto headers = GetHeaders(message.data);
            EXPECT_EQ("test", GetHeaderValue(HeaderName::kSession, headers));

            etl::string<128> response{"RTSP/1.0 "};
            response.append(response_status);
            response.append("\r\nCSeq: ");
            const auto cseq = GetHeaderValue(HeaderName::kCSeq, headers);
            ASSERT_LE(cseq.size() + 4, response.available());

            response.append(cseq);
            response.append("\r\n\r\n");
            server.SendBuffer(response);
            if(++_requests == 1) {
                ASSERT_LE(cseq.size(), _received_cseq.capacity());
                _received_cseq.assign(cseq);
                _received.set_value();
            }
        });
    }

protected:
    size_t _cseq = 0;
    etl::string<16> _received_cseq;
    MaintenanceClock _clock;
    ThreadPool _pool{2};

    std::unique_ptr<Server> _server;
    std::shared_ptr<Connection> _connection;
    std::shared_ptr<Session> _session;
    std::optional<Maintenance> _maintenance;

    std::atomic<size_t> _requests{0};
    std::promise<void> _received;
};

TEST_F(RtspMaintenanceTest, SessionTimeout) {
    Response response{
        .status_code = 200,
        .reason_phrase = "OK",
        .headers = {
            {HeaderName::kSession, "test;timeout=2"}
        }};
    EXPECT_EQ(2s, ParseSessionTimeout(response));

    response.headers[0].value = "test";
    EXPECT_EQ(60s, ParseSessionTimeout(response));

    const std::array<etl::string_view, 5> invalid_timeout_values = {
        "test;timeout=0",
        "test;timeout=-1",
        "test;timeout=abc",
        "test;timeout=1;timeout=2",
        "test;timeout=999999999999999999999999999999"
    };
    for(auto& value : invalid_timeout_values) {
        response.headers[0].value = value;
        EXPECT_FALSE(ParseSessionTimeout(response));
    }
}

TEST_F(RtspMaintenanceTest, KeepaliveAndStop) {
    StartServer();
    CreateMaintenance(10);

    auto ready = _received.get_future();
    _maintenance->Start(*_session, "rtsp://127.0.0.1/video", "test", kSec);
    _maintenance->Process();
    EXPECT_EQ(0, _requests.load());

    _clock.Add(kSec / 2);
    _maintenance->Process();
    EXPECT_EQ(std::future_status::ready, ready.wait_for(2s));

    _maintenance->Stop();
    const auto count = _requests.load();

    _clock.Add(kSec);
    _maintenance->Process();
    EXPECT_EQ(count, _requests);
}

TEST_F(RtspMaintenanceTest, RejectedKeepaliveReportsFailure) {
    StartServer("454 Session Not Found");
    CreateMaintenance();
    auto ready = _received.get_future();
    _maintenance->Start(*_session, "rtsp://127.0.0.1/video", "test", kSec);
    _clock.Add(kSec / 2);
    _maintenance->Process();
    ASSERT_EQ(std::future_status::ready, ready.wait_for(2s));
    bool rejected = false;
    for(size_t i = 0; (i < 200) && !rejected; ++i) {
        try { _maintenance->Process(); }
        catch(const std::runtime_error&) { rejected = true; }
        if(!rejected) { std::this_thread::sleep_for(1ms); }
    }
    EXPECT_TRUE(rejected);
}

TEST_F(RtspMaintenanceTest, StopBeforeStart) {
    StartServer();
    CreateMaintenance();
    _maintenance->Stop();
    _maintenance->Stop();
    _maintenance->Process();
    EXPECT_EQ(0, _cseq);
}

TEST_F(RtspMaintenanceTest, SharesSequenceCounterWithCaller) {
    StartServer();
    CreateMaintenance(10);

    auto ready = _received.get_future();
    EXPECT_EQ(11, ++_cseq);

    _maintenance->Start(*_session, "rtsp://127.0.0.1/video", "test", kSec);
    _clock.Add(kSec / 2);
    _maintenance->Process();
    ASSERT_EQ(std::future_status::ready, ready.wait_for(2s));
    EXPECT_EQ("12", _received_cseq);

    _maintenance->Stop();
    EXPECT_EQ(13, ++_cseq);
}

}
