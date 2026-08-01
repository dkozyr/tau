#pragma once

#include "apps/signalling/Storage.h"
#include "tau/ws/Connection.h"
#include <memory>
#include <optional>

namespace tau::signalling {

class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
    ClientSession(Storage& storage, ws::ConnectionWeakPtr connection);
    static std::shared_ptr<ClientSession> CreateAndStart(Storage& storage, ws::ConnectionPtr connection);

    ws::String Process(ws::String&& request);

    void SetSdpOffer(message::Payload&& payload);
    void SetIceCandidates(message::Payload&& payload);

    bool IsActive() const;
    std::optional<SessionId> GetStreamId() const { return _stream_id; }
    std::optional<SessionId> GetSessionId() const { return _session_id; }

private:
    Storage& _storage;
    ws::ConnectionWeakPtr _connection;

    std::optional<StreamId> _stream_id;
    std::optional<SessionId> _session_id;
};

using ClientSessionPtr = std::shared_ptr<ClientSession>;
using ClientSessionWeakPtr = std::weak_ptr<ClientSession>;

}
