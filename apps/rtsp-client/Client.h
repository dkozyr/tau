#pragma once

#include "apps/rtsp-client/Session.h"
#include "tau/rtsp/Request.h"
#include "tau/rtsp/Response.h"
#include "tau/sdp/Sdp.h"
#include "tau/net/Uri.h"

namespace tau::rtsp {

//https://datatracker.ietf.org/doc/html/rfc2326#appendix-D.1
class Client {
public:
    struct Options {
        net::Uri uri;
    };

public:
    Client(Executor executor, Options&& options);

    void SendRequestOptions();
    void SendRequestDescribe();
    void SendRequestSetup();
    void SendRequestPlay();
    void SendRequestTeardown();

private:
    Response SendRequestAndValidateResponse(Request&& request, const etl::string_view& cseq);
    std::optional<Response> SendRequest(Request&& request);

    void ParseAndValidateSdp(const etl::string_view& sdp_str);
    Session::Options CreateSessionOptions() const;

    static etl::string<64> CreateUriString(const Options& options);

private:
    Executor _executor;
    const etl::string<64> _uri;
    asio::ip::tcp::resolver::results_type _endpoints;
    size_t _cseq = 0;
    etl::string<2048> _text;

    std::optional<Session> _session;
    std::optional<uint16_t> _server_rtp_port;
    etl::string<16> _session_id;
    sdp::SdpPtr _sdp;
};

}
