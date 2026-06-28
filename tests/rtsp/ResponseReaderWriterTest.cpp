#include "tau/rtsp/ResponseWriter.h"
#include "tau/rtsp/ResponseReader.h"
#include "tests/lib/Common.h"

namespace tau::rtsp {

class ResponseReaderWriterTest : public ::testing::Test {
protected:
    static void ParseAndAssertResponse(const Response& target, etl::string_view message) {
        auto parsed = ResponseReader::Read(message);
        ASSERT_TRUE(parsed.has_value());
        const auto& actual = *parsed;
        ASSERT_EQ(target.status_code, actual.status_code);
        ASSERT_EQ(target.reason_phrase, actual.reason_phrase);
        ASSERT_EQ(target.headers.size(), actual.headers.size());
        for(size_t i = 0; i < target.headers.size(); ++i) {
            ASSERT_EQ(target.headers[i].name, actual.headers[i].name);
            ASSERT_EQ(target.headers[i].value, actual.headers[i].value);
        }
        ASSERT_EQ(target.body, actual.body);
    }

protected:
    etl::string<1024> _buffer;
};

TEST_F(ResponseReaderWriterTest, Options) {
    Response response{
        .status_code = 200,
        .reason_phrase = "OK",
        .headers = {}
    };
    etl::string<8> cseq;
    etl::to_string(g_random.Int<size_t>(1, 1234), cseq);
    response.headers.push_back({.name = HeaderName::kCSeq, .value = cseq});
    response.headers.push_back({.name = HeaderName::kPublic, .value = "OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, GET_PARAMETER, SET_PARAMETER"});

    auto& message = ResponseWriter::Write(response, _buffer);
    TAU_LOG_INFO(kClRf << message);
    ASSERT_NO_FATAL_FAILURE(ParseAndAssertResponse(response, message));
}

TEST_F(ResponseReaderWriterTest, Describe) {
    Response response{
        .status_code = 200,
        .reason_phrase = "OK",
        .headers = {}
    };
    etl::string<8> cseq;
    etl::to_string(g_random.Int<size_t>(1, 1234), cseq);
    response.headers.push_back({.name = HeaderName::kCSeq, .value = cseq});
    response.headers.push_back({.name = HeaderName::kContentBase, .value = "rtsp://192.168.0.1/stream.h264/"});
    response.headers.push_back({.name = HeaderName::kContentType, .value = "application/sdp"});
    response.headers.push_back({.name = HeaderName::kContentLength, .value = "495"});
    response.body = "v=0\r\no=- 1741809999999999 1 IN IP4 192.168.0.1\r\ns=Session\r\ni=stream.h264\r\nt=0 0\r\na=tool:Tool v2025.03.13\r\na=type:broadcast\r\na=control:*\r\na=range:npt=now-\r\nm=video 0 RTP/AVP 96\r\nc=IN IP4 0.0.0.0\r\nb=AS:700\r\na=rtpmap:96 H264/90000\r\na=fmtp:96 packetization-mode=1;profile-level-id=640020\r\n";

    auto& message = ResponseWriter::Write(response, _buffer);
    TAU_LOG_INFO(kClRf << message);
    ASSERT_NO_FATAL_FAILURE(ParseAndAssertResponse(response, message));
}

TEST_F(ResponseReaderWriterTest, Setup) {
    Response response{
        .status_code = 200,
        .reason_phrase = "OK"
    };
    etl::string<8> cseq;
    etl::to_string(g_random.Int<size_t>(1, 1234), cseq);
    response.headers.push_back({.name = HeaderName::kCSeq, .value = cseq});
    response.headers.push_back({.name = HeaderName::kTransport, .value = "RTP/AVP;unicast;destination=192.168.0.1;source=192.168.0.1;client_port=59882-59883;server_port=12345-12346"});
    response.headers.push_back({.name = HeaderName::kSession, .value = "E0E094D0;timeout=65"});

    auto& message = ResponseWriter::Write(response, _buffer);
    TAU_LOG_INFO(kClRf << message);
    ASSERT_NO_FATAL_FAILURE(ParseAndAssertResponse(response, message));
}

TEST_F(ResponseReaderWriterTest, Play) {
    Response response{
        .status_code = 200,
        .reason_phrase = "OK"
    };
    etl::string<8> cseq;
    etl::to_string(g_random.Int<size_t>(1, 1234), cseq);
    response.headers.push_back({.name = HeaderName::kCSeq, .value = cseq});
    response.headers.push_back({.name = HeaderName::kSession, .value = "E0E094D0;timeout=65"});
    response.headers.push_back({.name = HeaderName::kRtpInfo, .value = "url=rtsp://192.168.0.1/stream.h264/track1;seq=926;rtptime=810886235"});

    auto& message = ResponseWriter::Write(response, _buffer);
    TAU_LOG_INFO(kClRf << message);
    ASSERT_NO_FATAL_FAILURE(ParseAndAssertResponse(response, message));
}

TEST_F(ResponseReaderWriterTest, Teardown) {
    Response response{
        .status_code = 200,
        .reason_phrase = "OK"
    };
    etl::string<8> cseq;
    etl::to_string(g_random.Int<size_t>(1, 1234), cseq);
    response.headers.push_back({.name = HeaderName::kCSeq, .value = cseq});

    auto& message = ResponseWriter::Write(response, _buffer);
    TAU_LOG_INFO(kClRf << message);
    ASSERT_NO_FATAL_FAILURE(ParseAndAssertResponse(response, message));
}

TEST_F(ResponseReaderWriterTest, ReadCaseInsensitivePrefixes) {
    etl::string_stream ss(_buffer);
    ss << "RTSP/1.0 200 OK" << kClRf
       << "PUBLIC: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY" << kClRf
       << "cSeQ: 313" << kClRf
       << kClRf;
    auto& message = ss.str();
    auto response = ResponseReader::Read(message);
    ASSERT_TRUE(response.has_value());
    const auto& headers = response->headers;
    ASSERT_EQ(2, headers.size());
    ASSERT_EQ(HeaderName::kPublic, headers[0].name);
    ASSERT_EQ("OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY", headers[0].value);
    ASSERT_EQ(HeaderName::kCSeq, headers[1].name);
    ASSERT_EQ("313", headers[1].value);
}

}
