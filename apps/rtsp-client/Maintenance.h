#pragma once

#include "apps/rtsp-client/Session.h"
#include "tau/common/Clock.h"
#include "tau/rtsp/Connection.h"
#include "tau/asio/Timer.h"
#include <atomic>

namespace tau::rtsp {

class Maintenance : public std::enable_shared_from_this<Maintenance> {
public:
    struct Dependencies {
        Executor executor;
        Clock& clock;
    };

    struct Options {
        std::weak_ptr<Connection> connection;
        std::weak_ptr<Session> session;
        size_t cseq;
        etl::string_view uri;
        etl::string_view session_id;
        Timepoint timeout = 0;
    };

    using ConnectionWithSession = std::pair<std::shared_ptr<Connection>, std::shared_ptr<Session>>;

public:
    Maintenance(Dependencies deps, Options options);

    void Start();
    void Stop();

private:
    void StopInternal();

    void Schedule();
    void OnTimer(boost_ec ec);

    ConnectionWithSession GetConnectionWithSession(boost_ec ec);

private:
    Clock& _clock;
    
    std::mutex _mutex;
    Timer _timer;

    std::weak_ptr<Connection> _connection;
    std::weak_ptr<Session> _session;

    std::optional<std::future<StreamReader::Message>> _response;
    size_t _cseq;
    etl::string<256> _uri;
    etl::string<16> _session_id;

    Timepoint _interval;
    Timepoint _next_keepalive = 0;

    bool _started = false;
    bool _stopped = false;
};

}
