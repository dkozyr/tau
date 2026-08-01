#pragma once

#include "apps/signalling-server/DeviceSession.h"
#include "apps/signalling-server/ClientSession.h"
#include "apps/signalling/Storage.h"
#include "tau/ws/Server.h"
#include "tau/asio/ThreadPool.h"
#include "tau/asio/PeriodicTimer.h"
#include "tau/common/Log.h"
#include <optional>
#include <deque>
#include <unordered_map>
#include <mutex>

namespace tau::signalling {

class Server {
public:
    static constexpr auto kSessionTimeoutMs = 500;

    struct Options {
        etl::string_view host;
        uint16_t port;
        SslContext& ssl_ctx;
    };

public:
    Server();
    ~Server();

    void Start(const Options& options);
    void Stop();

private:
    void InitStorage();
    void OnTimer();

    ws::String InitSessionAndProcessRequest(ws::ConnectionPtr connection, ws::String&& request);

    DeviceSessionPtr GetDeviceById(DeviceId device_id);
    ClientSessionPtr GetClientBySessionId(SessionId session_id);

    std::vector<StreamId> ProcessInactiveAndGetStreamsToRemove();
    std::vector<std::pair<StreamId, SessionId>> ProcessInactiveAndGetSessionsToRemove();

private:
    ThreadPool _io;
    PeriodicTimer _timer;

    std::optional<ws::Server> _server;
    Storage _storage;

    std::mutex _mutex;
    std::deque<DeviceSessionPtr> _devices;
    std::unordered_map<DeviceId, DeviceSessionPtr> _device_by_id;

    std::deque<ClientSessionPtr> _clients;
    std::unordered_map<SessionId, ClientSessionPtr> _client_by_session_id;
};

}
