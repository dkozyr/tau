#pragma once

#include "apps/rtsp-client/Session.h"
#include "apps/rtsp-client/SessionTcp.h"
#include "apps/rtsp-client/Maintenance.h"
#include "tau/rtsp/Request.h"
#include "tau/rtsp/Response.h"
#include "tau/rtsp/Transport.h"
#include "tau/rtsp/Connection.h"
#include "tau/sdp/Sdp.h"
#include "tau/net/Uri.h"
#include "tau/asio/Timer.h"
#include "tau/common/SteadyClock.h"
#include <atomic>

namespace tau::rtsp {

//https://datatracker.ietf.org/doc/html/rfc2326#appendix-D.1
class Client {
    using UriStr = etl::string<256>;

public:
    struct Options {
        net::Uri uri;
        Transport transport = Transport::kUdp;
        Timepoint request_timeout = 5 * kSec;
    };

    using VideoCallback = std::function<void(Buffer&& nal_unit)>;

public:
    Client(Executor executor, Options&& options);
    ~Client();

    void SetVideoCallback(VideoCallback callback);

    void SendRequestOptions();
    void SendRequestDescribe();
    void SendRequestSetup();
    void SendRequestPlay();
    void SendRequestTeardown();

    // Close may cancel a pending OPTIONS request. Join request caller before destruction
    void Close();
    bool IsClosed() const;

private:
    void ApplySetupResponse(const Response& response);

    Response SendRequestAndValidateResponse(Request&& request, const etl::string_view& cseq);
    std::optional<Response> SendRequest(Request&& request);

    void ParseAndValidateSdp(const etl::string_view& sdp_str);
    Session::Options CreateSessionOptions() const;

    static UriStr CreateUriString(const Options& options);

private:
    Executor _executor;
    Strand _strand;
    Timer _timer;
    SteadyClock _clock;
    const UriStr _uri;
    UriStr _setup_uri;
    const Transport _transport;
    const Timepoint _request_timeout;

    Connection _connection;
    size_t _cseq = 0;
    std::optional<Maintenance> _maintenance;
    bool _closed = false;
    std::atomic_bool _transport_closed{false};

    StreamReader::String _text;

    std::optional<Session> _session;
    std::optional<SessionTcp> _session_tcp;
    std::optional<uint16_t> _server_rtp_port;
    std::optional<InterleavedChannels> _interleaved_channels;
    etl::string<16> _session_id;
    sdp::SdpPtr _sdp;

    VideoCallback _video_callback;
};

}
