#pragma once

#include "apps/rtsp-client/Session.h"
#include "tau/rtsp/Request.h"
#include "tau/rtsp/Response.h"
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
    Response SendRequestAndValidateResponse(Request&& request, const std::string& cseq);
    std::optional<Response> SendRequest(Request&& request);

private:
    Executor _executor;
    const std::string _uri;
    asio_tcp::resolver::results_type _endpoints;
    size_t _cseq = 0;

    std::optional<Session> _session;
    std::optional<uint16_t> _server_rtp_port;
    std::string _session_id;
};

}
