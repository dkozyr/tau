#include "apps/signalling-server/ClientSession.h"
#include "apps/signalling/message/Device.h"
#include "tau/asio/ToString.h"
#include "tau/common/Json.h"
#include "tau/common/Log.h"

namespace tau::signalling {

ClientSession::ClientSession(Storage& storage, ws::ConnectionWeakPtr connection)
    : _storage(storage)
    , _connection(std::move(connection))
{}

std::shared_ptr<ClientSession> ClientSession::CreateAndStart(Storage& storage, ws::ConnectionPtr connection) {
    auto session = std::make_shared<ClientSession>(storage, ws::ConnectionWeakPtr{connection});
    connection->SetProcessMessageCallback(
        [weak_self = std::weak_ptr<ClientSession>(session)](ws::String&& request) mutable {
            if(auto self = weak_self.lock()) {
                return self->Process(std::move(request));
            }
            return ws::String{};
        });
    return session;
}

ws::String ClientSession::Process(ws::String&& request) {
    boost_ec ec;
    auto parsed = Json::parse(request.data(), ec);
    if(ec) {
        TAU_LOG_WARNING("Failed to parse: " << ec);
        return R"({"error":"Failed to parse"})";
    }

    auto client = message::ClientFromJson(parsed.as_object());
    if(!client) {
        return R"({"error":"Failed to parse client message"})";
    }

    const auto notification = _storage.ProcessMessage(std::move(*client));
    TAU_LOG_INFO("Client notification: " << notification);

    if(!notification.session_id) {
        return R"({"error":"Failed to process client message"})";
    }
    // if(!ProcessResult(result)) {
    //     return R"({"error":"Failed to process client message"})";
    // }

    if(!_session_id) {
        _stream_id = client->stream_id;
        _session_id = notification.session_id;
    } else if(_session_id != notification.session_id) {
        return R"({"error":"Wrong session id"})"; 
    }
    TAU_LOG_INFO("_session_id: " << _session_id.value_or(0));

    request.clear();
    etl::string_stream ss(request);
    message::ClientNotificationToJson(ss, notification);
    return request;
}

void ClientSession::SetSdpOffer(message::Payload&& payload) {
    if(!_session_id) {
        TAU_LOG_WARNING("No session id");
        return;
    }

    if(auto connection = _connection.lock()) {
        message::ClientNotification notification{
            .session_id = *_session_id,
            .session_state = SessionState::kSdpOffered,
            .payload = std::move(payload)
        };
        ws::String message;
        etl::string_stream ss(message);
        message::ClientNotificationToJson(ss, notification);
        connection->PostMessage(std::move(message));
    } else {
        TAU_LOG_WARNING("No connection");
    }
}

void ClientSession::SetIceCandidates(message::Payload&& payload) {
    if(!_session_id) {
        TAU_LOG_WARNING("No session id");
        return;
    }

    if(auto connection = _connection.lock()) {
        message::ClientNotification notification{
            .session_id = *_session_id,
            .session_state = SessionState::kSdpAnswered,
            .payload = std::move(payload)
        };
        ws::String message;
        etl::string_stream ss(message);
        message::ClientNotificationToJson(ss, notification);
        connection->PostMessage(std::move(message));
    } else {
        TAU_LOG_WARNING("No connection");
    }
}

bool ClientSession::IsActive() const {
    return (_connection.lock() != nullptr);
}

}
