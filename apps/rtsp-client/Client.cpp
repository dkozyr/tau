#include "apps/rtsp-client/Client.h"
#include "apps/rtsp-client/Utils.h"
#include "tau/rtsp/RequestWriter.h"
#include "tau/rtsp/ResponseReader.h"
#include "tau/memory/SystemAllocator.h"
#include "tau/asio/ToString.h"
#include "tau/common/Exception.h"
#include "tau/common/Log.h"
#include <array>

namespace tau::rtsp {

Client::Client(Executor executor, Options&& options)
    : _executor(std::move(executor))
    , _uri(CreateUriString(options))
    , _endpoints(asio::ip::tcp::resolver(_executor).resolve(options.uri.host.data(), std::to_string(options.uri.port))) {
    for(auto& endpoint : _endpoints) {
        TAU_LOG_INFO("RTSP endpoint: " << endpoint.endpoint());
    }
}

void Client::SendRequestOptions() {
    _cseq++;
    const auto cseq = ToString<8>(_cseq);
    SendRequestAndValidateResponse(
        Request{
            .uri = _uri,
            .method = Method::kOptions,
            .headers {
                Header{.name = HeaderName::kCSeq, .value = cseq},
            }
        },
        cseq);
}

void Client::SendRequestDescribe() {
    _cseq++;
    const auto cseq = ToString<8>(_cseq);
    const auto response = SendRequestAndValidateResponse(
        Request{
            .uri = _uri,
            .method = Method::kDescribe,
            .headers {
                Header{.name = HeaderName::kCSeq, .value = cseq},
            }
        },
        cseq);
    TAU_LOG_INFO("RTSP SDP:\r\n" << response.body);
    ParseAndValidateSdp(response.body);
}

void Client::SendRequestSetup() {
    _session.emplace(_executor, CreateSessionOptions());
    const auto rtp_port = _session->GetRtpPort();
    const auto rtcp_port = rtp_port + 1;
    TAU_LOG_INFO("Rtp port: " << rtp_port << ", rtcp port: " << rtcp_port);

    _cseq++;
    const auto cseq = ToString<8>(_cseq);
    etl::string<64> value;
    value.append("RTP/AVP/UDP;unicast;client_port=");
    value.append(ToString<8>(rtp_port));
    value.append("-");
    value.append(ToString<8>(rtcp_port));
    auto response = SendRequestAndValidateResponse(
        Request{
            .uri = _uri,
            .method = Method::kSetup,
            .headers {
                Header{.name = HeaderName::kCSeq, .value = cseq},
                Header{.name = HeaderName::kTransport, .value = value}
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
    const auto cseq = ToString<8>(_cseq);
    SendRequestAndValidateResponse(
        Request{
            .uri = _uri,
            .method = Method::kPlay,
            .headers {
                Header{.name = HeaderName::kCSeq, .value = cseq},
                Header{.name = HeaderName::kSession, .value = _session_id},
            }
        },
        cseq);
}

void Client::SendRequestTeardown() {
    _cseq++;
    const auto cseq = ToString<8>(_cseq);
    SendRequestAndValidateResponse(
        Request{
            .uri = _uri,
            .method = Method::kTeardown,
            .headers {
                Header{.name = HeaderName::kCSeq, .value = cseq},
                Header{.name = HeaderName::kSession, .value = _session_id},
            }
        },
        cseq);
}

Response Client::SendRequestAndValidateResponse(Request&& request, const etl::string_view& cseq) {
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
    asio::ip::tcp::socket socket(_executor);
    asio::connect(socket, _endpoints, ec);
    if(ec) {
        TAU_LOG_WARNING("connect error: " << ec);
        return {};
    }

    RequestWriter::Write(request, _text);
    TAU_LOG_INFO("Request:\r\n" << _text);
    asio::write(socket, asio::buffer(_text.data(), _text.size()), ec);
    if(ec) {
        TAU_LOG_WARNING("write error: " << ec);
        return {};
    }

    _text.clear();
    std::array<char, 256> buffer;
    do {
        auto bytes = socket.read_some(asio::buffer(buffer), ec);
        if(ec) {
            if(ec != asio::error::eof) {
                TAU_LOG_WARNING("read_some error: " << ec);
            }
            break;
        }
        _text.append(buffer.data(), bytes);
    } while(socket.available() > 0);
    socket.close(ec);

    TAU_LOG_INFO("Response:\r\n" << _text);
    return ResponseReader::Read(_text);
}

void Client::ParseAndValidateSdp(const etl::string_view& sdp_str) {
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
        SplitTokens<16> tokens;
        Split(tokens, codec.format, ";");
        for(auto& token : tokens) {
            const etl::string_view kPrefix = "sprop-parameter-sets="; //TODO: name and move to h264 or sdp namespace?
            if(IsPrefix(token, kPrefix)) {
                SplitTokens<2> params;
                Split(params, token.substr(kPrefix.size()), ",");
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

etl::string<64> Client::CreateUriString(const Options& options) {
    etl::string<64> uri;
    uri.append("rtsp://");
    uri.append(options.uri.host);
    uri.append("/");
    uri.append(options.uri.path);
    return uri;
}

}
