#include "tau/rtsp/RequestWriter.h"
#include "tau/rtsp/RequestReader.h"
#include "tests/lib/Common.h"

namespace tau::rtsp {

class RequestReaderWriterTest : public ::testing::Test {
protected:
    static void ParseAndAssertRequest(const Request& target, etl::string_view message) {
        auto parsed = RequestReader::Read(message);
        ASSERT_TRUE(parsed.has_value());
        const auto& actual = *parsed;
        ASSERT_EQ(target.method, actual.method);
        ASSERT_EQ(target.uri, actual.uri);
        ASSERT_EQ(target.headers.size(), actual.headers.size());
        for(size_t i = 0; i < target.headers.size(); ++i) {
            ASSERT_EQ(target.headers[i].name, actual.headers[i].name);
            ASSERT_EQ(target.headers[i].value, actual.headers[i].value);
        }
    }

protected:
    etl::string<1024> _buffer;
};

TEST_F(RequestReaderWriterTest, Options) {
    Request request{
        .uri = "*",
        .method = Method::kOptions,
        .headers = {}
    };
    etl::string<4> cseq;
    etl::to_string(g_random.Int<size_t>(1, 1234), cseq);
    request.headers.push_back({.name = HeaderName::kCSeq, .value = cseq});

    auto& message = RequestWriter::Write(request, _buffer);
    TAU_LOG_INFO(kClRf << message);
    ASSERT_NO_FATAL_FAILURE(ParseAndAssertRequest(request, message));
}

TEST_F(RequestReaderWriterTest, Describe) {
    Request request{
        .uri = "rtsp://127.0.0.1/stream",
        .method = Method::kDescribe,
        .headers = {}
    };
    etl::string<4> cseq;
    etl::to_string(g_random.Int<size_t>(1, 1234), cseq);
    request.headers.push_back({.name = HeaderName::kCSeq, .value = cseq});
    request.headers.push_back({.name = HeaderName::kAccept, .value = "application/sdp"});

    auto& message = RequestWriter::Write(request, _buffer);
    TAU_LOG_INFO(kClRf << message);
    ASSERT_NO_FATAL_FAILURE(ParseAndAssertRequest(request, message));
}

TEST_F(RequestReaderWriterTest, Setup) {
    Request request{
        .uri = "rtsp://127.0.0.1/stream",
        .method = Method::kSetup,
        .headers = {}
    };
    etl::string<4> cseq;
    etl::to_string(g_random.Int<size_t>(1, 1234), cseq);
    request.headers.push_back({.name = HeaderName::kCSeq, .value = cseq});
    request.headers.push_back({.name = HeaderName::kTransport, .value = "RTP/AVP;unicast;client_port=12345-12346"});

    auto& message = RequestWriter::Write(request, _buffer);
    TAU_LOG_INFO(kClRf << message);
    ASSERT_NO_FATAL_FAILURE(ParseAndAssertRequest(request, message));
}

TEST_F(RequestReaderWriterTest, Play) {
    Request request{
        .uri = "rtsp://127.0.0.1/stream",
        .method = Method::kPlay,
        .headers = {}
    };
    etl::string<4> cseq;
    etl::to_string(g_random.Int<size_t>(1, 1234), cseq);
    auto session_id = ToHexString(g_random.Int<size_t>());
    request.headers.push_back({.name = HeaderName::kCSeq, .value = cseq});
    request.headers.push_back({.name = HeaderName::kSession, .value = session_id});

    auto& message = RequestWriter::Write(request, _buffer);
    TAU_LOG_INFO(kClRf << message);
    ASSERT_NO_FATAL_FAILURE(ParseAndAssertRequest(request, message));
}

TEST_F(RequestReaderWriterTest, Teardown) {
    Request request{
        .uri = "rtsp://127.0.0.1/stream",
        .method = Method::kTeardown,
        .headers = {}
    };
    etl::string<4> cseq;
    etl::to_string(g_random.Int<size_t>(1, 1234), cseq);
    auto session_id = ToHexString(g_random.Int<size_t>());
    request.headers.push_back({.name = HeaderName::kCSeq, .value = cseq});
    request.headers.push_back({.name = HeaderName::kSession, .value = session_id});

    auto& message = RequestWriter::Write(request, _buffer);
    TAU_LOG_INFO(kClRf << message);
    ASSERT_NO_FATAL_FAILURE(ParseAndAssertRequest(request, message));
}

}
