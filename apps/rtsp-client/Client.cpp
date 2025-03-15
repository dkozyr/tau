#include "apps/rtsp-client/Client.h"
#include "apps/rtsp-client/Utils.h"
#include "tau/rtsp/RequestWriter.h"
#include "tau/rtsp/ResponseReader.h"
#include "tau/common/Log.h"
#include <sstream>
#include <array>

namespace tau::rtsp {

Client::Client(Executor executor, Options&& options)
    : _executor(std::move(executor))
    , _options(std::move(options)) {
    asio_tcp::resolver resolver(_executor);
    _endpoints = resolver.resolve(_options.host, std::to_string(_options.port));

    for(auto& endpoint : _endpoints) {
        LOG_INFO << endpoint.endpoint();
    }
}

void Client::SendRequestOptions() {
    _cseq++;
    const auto cseq = std::to_string(_cseq);
    SendRequestAndValidateResponse(
        Request{
            .uri = _options.uri,
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
            .uri = _options.uri,
            .method = Method::kDescribe,
            .headers {
                {.name = HeaderName::kCSeq, .value = cseq},
            }
        },
        cseq);
    LOG_INFO << "RTSP SDP:" << std::endl << response.body; //TODO: store/parse SDP
}

void Client::SendRequestSetup() {
    _session.emplace(_executor, Session::Options{});
    const auto rtp_port = _session->GetRtpPort();
    const auto rtcp_port = _session->GetRtcpPort();
    LOG_INFO << "Rtp port: " << rtp_port << ", rtcp port: " << rtcp_port;

    _cseq++;
    const auto cseq = std::to_string(_cseq);
    auto response = SendRequestAndValidateResponse(
        Request{
            .uri = _options.uri,
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
        throw std::runtime_error("Wrong SETUP response");
    }
    LOG_INFO << "RTSP server rtp port: " << *_server_rtp_port << ", session id: " << _session_id;
}

void Client::SendRequestPlay() {
    _cseq++;
    const auto cseq = std::to_string(_cseq);
    SendRequestAndValidateResponse(
        Request{
            .uri = _options.uri,
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
            .uri = _options.uri,
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
        throw std::runtime_error("Wrong response, request CSeq: " + cseq);
    }
    if(cseq != GetHeaderValue(HeaderName::kCSeq, response->headers)) {
        throw std::runtime_error("Wrong response CSeq, expected CSeq: " + cseq);
    }
    return *response;
}

std::optional<Response> Client::SendRequest(Request&& request) {
    boost_ec ec;
    asio_tcp::socket socket(_executor);
    asio::connect(socket, _endpoints, ec);
    if(ec) {
        LOG_WARNING << "connect error: " << ec.value() << ", ec: " << ec.message();
        return {};
    }

    const auto request_str = RequestWriter::Write(request);
    LOG_INFO << "Request: " << std::endl << request_str;
    asio::write(socket, asio::buffer(request_str), ec);
    if(ec) {
        LOG_WARNING << "write error: " << ec.value() << ", message: " << ec.message();
        return {};
    }

    std::string response;
    response.reserve(1024);
    std::array<char, 256> buffer;
    do {
        auto bytes = socket.read_some(asio::buffer(buffer), ec);
        if(ec) {
            if(ec != asio::error::eof) {
                LOG_WARNING << "read_some error: " << ec.value() << ", message: " << ec.message();
            }
            break;
        }
        response.append(buffer.data(), bytes);
    } while(socket.available() > 0);
    socket.close(ec);

    LOG_INFO << "Response: " << std::endl << response;
    return ResponseReader::Read(response);
}

}
