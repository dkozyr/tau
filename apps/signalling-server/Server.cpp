#include "apps/signalling-server/Server.h"
#include "tau/asio/ToString.h"
#include <string>

namespace tau::signalling {

Server::Server()
    : _io(std::thread::hardware_concurrency())
    , _timer(_io.GetExecutor())
{
    InitStorage();
}

Server::~Server() {
    Stop();
    _io.Join();
}

void Server::Start(const Options& options) {
    _server.emplace(
        ws::Server::Dependencies{.executor = _io.GetExecutor()},
        ws::Server::Options{options.host, options.port, options.ssl_ctx}
    );
    _server->SetOnNewConnectionCallback([this](ws::ConnectionPtr connection) {
        connection->SetProcessMessageCallback(
            [this, connection_weak = ws::ConnectionWeakPtr{connection}](ws::String&& request) -> ws::String {
                if(auto connection = connection_weak.lock()) {
                    return InitSessionAndProcessRequest(std::move(connection), std::move(request));
                }
                return {};
            });
    });
    _server->Start();
    _timer.Start(kSessionTimeoutMs, [this](boost_ec ec){
        if(ec) {
            return false;
        }
        OnTimer();
        return true;
    });
}

void Server::InitStorage() {
    _storage.SetDeviceMessageCallback([this](DeviceId device_id, SessionId session_id, SessionState session_state, message::Payload&& payload) {
        TAU_LOG_INFO("[device] Device_id: " << device_id << ", session_id: " << session_id << ", session_state: " << session_state << ", payload: " << payload);

        auto device_ptr = GetDeviceById(device_id);
        if(device_ptr) {
            if(session_state == SessionState::kClosed) {
                device_ptr->CloseSession(session_id);
                return;
            }

            switch(payload.type) {
                case message::PayloadType::kEmpty:
                    device_ptr->CreateSession(session_id);
                    break;
                case message::PayloadType::kSdp:
                    device_ptr->SetSdpAnswer(std::move(payload));
                    break;
                case message::PayloadType::kIceCandidates:
                    device_ptr->SetIceCandidates(std::move(payload));
                    break;
                default:
                    break;
            }
        } else {
            TAU_LOG_WARNING("Device not found");
        }
    });

    _storage.SetClientMessageCallback([this](DeviceId device_id, SessionId session_id, SessionState session_state, message::Payload&& payload) {
        TAU_LOG_INFO("[client] Device_id: " << device_id << ", session_id: " << session_id << ", session_state: " << session_state << ", payload: " << payload);

        //TODO: process closed state?

        auto client_ptr = GetClientBySessionId(session_id);
        if(client_ptr) {
            if(payload.type == message::PayloadType::kSdp) {
                client_ptr->SetSdpOffer(std::move(payload));
            }
        } else {
            TAU_LOG_WARNING("Client not found");
        }
    });
}

void Server::OnTimer() {
    auto streams_to_remove = ProcessInactiveAndGetStreamsToRemove();
    for(auto& stream_id : streams_to_remove) {
        TAU_LOG_INFO("Remove disconnected device, stream_id: " << stream_id)
        _storage.RemoveStream(stream_id);
    }

    auto sessions_to_remove = ProcessInactiveAndGetSessionsToRemove();
    for(auto& [stream_id, session_id] :  sessions_to_remove) {
        TAU_LOG_INFO("Remove disconnected client, stream_id: " << stream_id << ", session_id: " << session_id);
        _storage.RemoveSession(stream_id, session_id);
    }
}

std::vector<StreamId> Server::ProcessInactiveAndGetStreamsToRemove() {
    std::vector<StreamId> streams_to_remove;

    std::lock_guard lock{_mutex};
    for(auto it = _devices.begin(); it != _devices.end(); ) {
        auto& device = *it;
        if(!device->IsActive()) {
            const auto stream_id = device->GetStreamId();
            if(stream_id) {
                streams_to_remove.push_back(*stream_id);
            }
            it = _devices.erase(it);
        } else {
            it++;
        }
    }
    for(auto it = _device_by_id.begin(); it != _device_by_id.end(); ) {
        auto& device = it->second;
        if(!device->IsActive()) {
            const auto stream_id = device->GetStreamId();
            if(stream_id) {
                streams_to_remove.push_back(*stream_id);
            }
            it = _device_by_id.erase(it);
        } else {
            it++;
        }
    }
    return streams_to_remove;
}

std::vector<std::pair<StreamId, SessionId>> Server::ProcessInactiveAndGetSessionsToRemove() {
    std::vector<std::pair<StreamId, SessionId>> sessions_to_remove;

    std::lock_guard lock{_mutex};
    for(auto it = _clients.begin(); it != _clients.end(); ) {
        auto& client = *it;
        if(!client->IsActive()) {
            const auto stream_id = client->GetStreamId();
            const auto session_id = client->GetSessionId();
            if(stream_id && session_id) {
                sessions_to_remove.push_back(std::make_pair(*stream_id, *session_id));
            }
            it = _clients.erase(it);
        } else {
            it++;
        }
    }
    for(auto it = _client_by_session_id.begin(); it != _client_by_session_id.end(); ) {
        auto& client = it->second;
        if(!client->IsActive()) {
            const auto stream_id = client->GetStreamId();
            const auto session_id = client->GetSessionId();
            if(stream_id && session_id) {
                sessions_to_remove.push_back(std::make_pair(*stream_id, *session_id));
            }
            it = _client_by_session_id.erase(it);
        } else {
            it++;
        }
    }
    return sessions_to_remove;
}

ws::String Server::InitSessionAndProcessRequest(ws::ConnectionPtr connection, ws::String&& request) {
    auto target = connection->GetRequestTarget();
    target = target.substr(0, target.find('?'));

    ws::String response;
    if(target == "/device") {
        auto device_ptr = DeviceSession::CreateAndStart(_storage, std::move(connection));
        response = device_ptr->Process(std::move(request));
        {
            std::lock_guard lock{_mutex};
            _devices.push_back(std::move(device_ptr));
        }
    } else {
        auto client_ptr = ClientSession::CreateAndStart(_storage, std::move(connection));
        response = client_ptr->Process(std::move(request));
        {
            std::lock_guard lock{_mutex};
            _clients.push_back(std::move(client_ptr));
        }
    }
    return response;
}

void Server::Stop() {
    _timer.Stop();
    _server.reset();
}

DeviceSessionPtr Server::GetDeviceById(DeviceId device_id) {
    std::lock_guard lock{_mutex};
    auto it = _device_by_id.find(device_id);
    if(it != _device_by_id.end()) {
        return it->second;
    }
    for(auto it = _devices.begin(); it != _devices.end(); ++it) {
        auto session = *it;
        if(session->GetDeviceId() == device_id) {
            _device_by_id[device_id] = session;
            _devices.erase(it);
            return session;
        }
    }
    return nullptr;
}

ClientSessionPtr Server::GetClientBySessionId(SessionId session_id) {
    std::lock_guard lock{_mutex};
    auto it = _client_by_session_id.find(session_id);
    if(it != _client_by_session_id.end()) {
        return it->second;
    }
    for(auto it = _clients.begin(); it != _clients.end(); ++it) {
        auto session = *it;
        if(session->GetSessionId() == session_id) {
            _client_by_session_id[session_id] = session;
            _clients.erase(it);
            return session;
        }
    }
    return nullptr;
}

}
