#pragma once

#include "apps/rtsp-client/Session.h"
#include "tau/common/Clock.h"
#include "tau/rtsp/Connection.h"

namespace tau::rtsp {

class Maintenance {
public:
    Maintenance(Clock& clock, Connection& connection, size_t& cseq, std::weak_ptr<Session> session);

    void Start(etl::string_view uri, etl::string_view session_id, Timepoint timeout);
    void Stop();
    void Process();

private:
    Clock& _clock;
    Connection& _connection;
    size_t& _cseq;
    std::weak_ptr<Session> _session;

    bool _started = false;
    etl::string<256> _uri;
    etl::string<16> _session_id;

    Timepoint _interval = 0;
    Timepoint _next_keepalive = 0;

    std::optional<std::future<StreamReader::Message>> _response;
};

}
