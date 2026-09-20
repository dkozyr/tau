#include "apps/rtsp-client/Client.h"
#include "apps/rtsp-client/Utils.h"
#include "apps/rtsp-client/SetupUri.h"
#include "tau/rtsp/RequestWriter.h"
#include "tau/rtsp/ResponseReader.h"
#include "tau/memory/SystemAllocator.h"
#include "tau/asio/ToString.h"
#include "tau/common/Exception.h"
#include "tau/common/Log.h"
#include <cassert>

namespace tau::rtsp {

Client::Client(Executor executor, Options&& options)
    : _executor(std::move(executor))
    , _strand(asio::make_strand(_executor))
    , _timer(_strand)
    , _uri(CreateUriString(options))
    , _transport(options.transport)
    , _request_timeout(options.request_timeout)
    , _connection(_strand,
        Connection::Options{
            .host = options.uri.host,
            .port = options.uri.port,
            .timeout = std::chrono::milliseconds(options.request_timeout / kMs)
        })
{
    _connection.SetCloseCallback([this]() {
        _transport_closed.store(true);
    });
}

Client::~Client() {
    Close();
}

void Client::SetVideoCallback(VideoCallback callback) {
    assert(!_closed && !_session && !_session_tcp);
    _video_callback = std::move(callback);
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
    _setup_uri = CreateSetupUri(_uri, GetHeaderValue(HeaderName::kContentBase, response.headers), _sdp->medias[0].control);
}

void Client::SendRequestSetup() {
    assert(!_closed && _sdp && !_session && !_session_tcp && _session_id.empty());

    uint16_t rtp_port = 0;
    if(_transport == Transport::kUdp) {
        _session.emplace(_strand, CreateSessionOptions(), std::move(_video_callback));
        rtp_port = _session->GetRtpPort();
        TAU_LOG_INFO("Rtp port: " << rtp_port << ", rtcp port: " << rtp_port + 1);
    }
    const auto cseq = ToString<8>(++_cseq);
    const auto value = CreateTransportHeader(_transport, rtp_port);
    auto response = SendRequestAndValidateResponse(
        Request{
            .uri = _setup_uri,
            .method = Method::kSetup,
            .headers {
                Header{.name = HeaderName::kCSeq, .value = cseq},
                Header{.name = HeaderName::kTransport, .value = value}
            }
        },
        cseq);
    ApplySetupResponse(response);

    if(_transport == Transport::kTcp) {
        _session_tcp.emplace(_connection, SessionTcp::Options{
            .channels = *_interleaved_channels,
            .pipeline = CreateSessionOptions()
        });
        _session_tcp->SetVideoCallback(std::move(_video_callback));
    }
    TAU_LOG_INFO("RTSP session id: " << _session_id);
}

void Client::SendRequestPlay() {
    assert(!_closed && !_session_id.empty());

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

void Client::Close() {
    if(_closed) {
        return;
    }
    assert(!_strand.running_in_this_thread());

    std::promise<void> completion;
    auto shutdown = completion.get_future();
    asio::post(_strand, [this, &completion]() {
        if(_session) {
            _session->Stop();
        }
        _connection.AsyncClose([&completion]() { completion.set_value(); });
    });
    shutdown.get();

    if(_maintenance) {
        _maintenance->Stop();
    }
    _session.reset();
    _session_tcp.reset();
    _video_callback = {};
    _closed = true;
}

bool Client::IsClosed() const {
    return _transport_closed.load();
}

void Client::ApplySetupResponse(const Response& response) {
    const auto parameters = ParseTransport(GetHeaderValue(HeaderName::kTransport, response.headers));
    if(!parameters || (parameters->transport != _transport)) {
        TAU_EXCEPTION(std::runtime_error, "Invalid or mismatched SETUP transport");
    }
    const auto session_id = ParseSessionId(response);
    if(session_id.empty() || (session_id.size() > _session_id.capacity())) {
        TAU_EXCEPTION(std::runtime_error, "Invalid SETUP session id");
    }
    _server_rtp_port = parameters->server_rtp_port;
    _interleaved_channels = parameters->channels;
    _session_id = session_id;
}

Response Client::SendRequestAndValidateResponse(Request&& request, const etl::string_view& cseq) {
    auto response = SendRequest(std::move(request));
    if(!response) {
        TAU_EXCEPTION(std::runtime_error, "Wrong response, request CSeq: " << cseq);
    }
    if(cseq != GetHeaderValue(HeaderName::kCSeq, response->headers)) {
        TAU_EXCEPTION(std::runtime_error, "Wrong response CSeq, expected CSeq: " << cseq);
    }
    if((response->status_code < 200) || (response->status_code >= 300)) {
        TAU_EXCEPTION(std::runtime_error, "RTSP request failed, status: " << response->status_code);
    }
    return *response;
}

std::optional<Response> Client::SendRequest(Request&& request) {
    assert(!_closed && !_strand.running_in_this_thread());
    RequestWriter::Write(request, _text);
    TAU_LOG_INFO("Request:\r\n" << _text);
    const auto cseq = GetHeaderValue(HeaderName::kCSeq, request.headers);
    auto response = _connection.SendRequest(_text, cseq);
    _text = std::move(response.get().data);

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
        size_t pos = 0;
        while(pos != etl::string_view::npos) {
            auto token = SplitNext(codec.format, pos, ";");
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

Client::UriStr Client::CreateUriString(const Options& options) {
    UriStr uri;
    const auto port = ToString<8>(options.uri.port);
    uri.append("rtsp://");
    uri.append(options.uri.host);
    uri.append(":");
    uri.append(port);
    uri.append("/");
    uri.append(options.uri.path);
    if(uri.full()) {
        TAU_EXCEPTION(std::invalid_argument, "RTSP URI too long");
    }
    return uri;
}

}
