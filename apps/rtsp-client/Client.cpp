#include "apps/rtsp-client/Client.h"
#include "apps/rtsp-client/Utils.h"
#include "tau/rtsp/RequestWriter.h"
#include "tau/rtsp/ResponseReader.h"
#include "tau/memory/SystemAllocator.h"
#include "tau/common/Exception.h"
#include "tau/common/Log.h"
#include <array>

namespace tau::rtsp {

Client::Client(Executor executor, Options&& options)
    : _executor(std::move(executor))
    , _uri("rtsp://" + options.uri.host + "/" + options.uri.path)
    , _endpoints(asio_tcp::resolver(_executor).resolve(options.uri.host, std::to_string(options.uri.port))) {
    for(auto& endpoint : _endpoints) {
        TAU_LOG_INFO("RTSP endpoint: " << endpoint.endpoint());
    }
}

void Client::SendRequestOptions() {
    _cseq++;
    const auto cseq = std::to_string(_cseq);
    SendRequestAndValidateResponse(
        Request{
            .uri = _uri,
            .method = Method::kOptions,
            .headers {
                {.name = HeaderName::kCSeq, .value = cseq},
            }
        },
        cseq);
}

void Client::SendRequestDescribe() {
    _cseq++;
    const auto cseq = std::to_string(_cseq);
    const auto response = SendRequestAndValidateResponse(
        Request{
            .uri = _uri,
            .method = Method::kDescribe,
            .headers {
                {.name = HeaderName::kCSeq, .value = cseq},
            }
        },
        cseq);
    TAU_LOG_INFO("RTSP SDP:" << std::endl << response.body);
    ParseAndValidateSdp(response.body);
}

void Client::SendRequestSetup() {
    _session.emplace(_executor, CreateSessionOptions());
    const auto rtp_port = _session->GetRtpPort();
    const auto rtcp_port = rtp_port + 1;
    TAU_LOG_INFO("Rtp port: " << rtp_port << ", rtcp port: " << rtcp_port);

    _cseq++;
    const auto cseq = std::to_string(_cseq);
    auto response = SendRequestAndValidateResponse(
        Request{
            .uri = _uri,
            .method = Method::kSetup,
            .headers {
                {.name = HeaderName::kCSeq, .value = cseq},
                {.name = HeaderName::kTransport, .value = std::string{"RTP/AVP/UDP;unicast;client_port="} + std::to_string(rtp_port) + "-" + std::to_string(rtcp_port)}
            }
        },
        cseq);
    _server_rtp_port = ParseServerRtpPort(response);
    _session_id = ParseSessionId(response);
    if(!_server_rtp_port || _session_id.empty()) {
        TAU_EXCEPTION(std::runtime_error, "Wrong SETUP response");
    }
    TAU_LOG_INFO("RTSP server rtp port: " << *_server_rtp_port << ", session id: " << _session_id);
}

void Client::SendRequestPlay() {
    _cseq++;
    const auto cseq = std::to_string(_cseq);
    SendRequestAndValidateResponse(
        Request{
            .uri = _uri,
            .method = Method::kPlay,
            .headers {
                {.name = HeaderName::kCSeq, .value = cseq},
                {.name = HeaderName::kSession, .value = _session_id},
            }
        },
        cseq);
}

void Client::SendRequestTeardown() {
    _cseq++;
    const auto cseq = std::to_string(_cseq);
    SendRequestAndValidateResponse(
        Request{
            .uri = _uri,
            .method = Method::kTeardown,
            .headers {
                {.name = HeaderName::kCSeq, .value = cseq},
                {.name = HeaderName::kSession, .value = _session_id},
            }
        },
        cseq);
}

Response Client::SendRequestAndValidateResponse(Request&& request, const std::string& cseq) {
    auto response = SendRequest(std::move(request));
    if(!response) {
        TAU_EXCEPTION(std::runtime_error, "Wrong response, request CSeq: " << cseq);
    }
    if(cseq != GetHeaderValue(HeaderName::kCSeq, response->headers)) {
        TAU_EXCEPTION(std::runtime_error, "Wrong response CSeq, expected CSeq: " << cseq);
    }
    return *response;
}

std::optional<Response> Client::SendRequest(Request&& request) {
    boost_ec ec;
    asio_tcp::socket socket(_executor);
    asio::connect(socket, _endpoints, ec);
    if(ec) {
        TAU_LOG_WARNING("connect error: " << ec.value() << ", ec: " << ec.message());
        return {};
    }

    const auto request_str = RequestWriter::Write(request);
    TAU_LOG_INFO("Request: " << std::endl << request_str);
    asio::write(socket, asio::buffer(request_str), ec);
    if(ec) {
        TAU_LOG_WARNING("write error: " << ec.value() << ", message: " << ec.message());
        return {};
    }

    std::string response;
    response.reserve(1024);
    std::array<char, 256> buffer;
    do {
        auto bytes = socket.read_some(asio::buffer(buffer), ec);
        if(ec) {
            if(ec != asio::error::eof) {
                TAU_LOG_WARNING("read_some error: " << ec.value() << ", message: " << ec.message());
            }
            break;
        }
        response.append(buffer.data(), bytes);
    } while(socket.available() > 0);
    socket.close(ec);

    TAU_LOG_INFO("Response: " << std::endl << response);
    return ResponseReader::Read(response);
}

void Client::ParseAndValidateSdp(const std::string_view& sdp_str) {
    _sdp = sdp::ParseSdp(sdp_str);
    if(!_sdp) {
        TAU_EXCEPTION(std::runtime_error, "Sdp parsing failed");
    }
    if(_sdp->medias.size() != 1) {
        TAU_EXCEPTION(std::runtime_error, "Sdp processing failed: expected only 1 media");
    }
    const auto& video = _sdp->medias[0];
    if(video.type != sdp::MediaType::kVideo) {
        TAU_EXCEPTION(std::runtime_error, "Sdp processing failed: expected video media");
    }
    const auto& [_, codec] = *video.codecs.begin();
    if(codec.name != "H264") {
        TAU_EXCEPTION(std::runtime_error, "Sdp processing failed: expected H264 video media");
    }
}

Session::Options Client::CreateSessionOptions() const {
    const auto& video = _sdp->medias[0];
    const auto& [_, codec] = *video.codecs.begin();
    if(!codec.format.empty()) {
        const auto tokens = Split(codec.format, ";");
        for(auto& token : tokens) {
            const std::string_view kPrefix = "sprop-parameter-sets="; //TODO: name and move to h264 or sdp namespace?
            if(IsPrefix(token, kPrefix)) {
                const auto params = Split(token.substr(kPrefix.size()), ",");
                if(params.size() == 2) {
                    return Session::Options{
                        .clock_rate = codec.clock_rate,
                        .sps = CreateBufferFromBase64(g_system_allocator, params[0]),
                        .pps = CreateBufferFromBase64(g_system_allocator, params[1])
                    };
                }
            }
        }
    }
    return Session::Options{
        .clock_rate = codec.clock_rate,
    };
}

}
