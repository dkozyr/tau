#include "apps/signalling-server/DeviceSession.h"
#include "apps/signalling/message/Device.h"
#include "apps/signalling/message/DeviceNotification.h"
#include "tau/asio/ToString.h"
#include "tau/common/Json.h"
#include "tau/common/Log.h"

namespace tau::signalling {

DeviceSession::DeviceSession(Storage& storage, ws::ConnectionWeakPtr connection)
    : _storage(storage)
    , _connection(std::move(connection))
{}

std::shared_ptr<DeviceSession> DeviceSession::CreateAndStart(Storage& storage, ws::ConnectionPtr connection) {
    auto session = std::make_shared<DeviceSession>(storage, ws::ConnectionWeakPtr{connection});
    connection->SetProcessMessageCallback(
        [weak_self = std::weak_ptr<DeviceSession>(session)](ws::String&& request) mutable {
            if(auto self = weak_self.lock()) {
                return self->Process(std::move(request));
            }
            return ws::String{};
        });
    return session;
}

ws::String DeviceSession::Process(ws::String&& request) {
    boost_ec ec;
    auto parsed = Json::parse(request.data(), ec);
    if(ec) {
        TAU_LOG_WARNING("Failed to parse: " << ec);
        return R"({"error":"Failed to parse"})";
    }

    auto device = message::DeviceFromJson(parsed.as_object());
    if(!device) {
        return R"({"error":"Failed to parse device message"})";
    }
    if(!_device_id) {
        _device_id = device->device_id;
    } else if(_device_id != device->device_id) {
        return R"({"error":"Wrong device id"})"; 
    }

    //TODO: do we need it?
    if(_state == State::kWaiting) {
        _state = State::kReady;
    }

    const auto notification = _storage.ProcessMessage(std::move(*device));
    TAU_LOG_INFO("Device notification: " << notification);

    if(!notification.stream_id) {
        return R"({"error":"Failed to process device message"})";
    }
    if(!ProcessNotification(notification)) {
        return R"({"error":"Failed to process device message"})";
    }

    request.clear();
    etl::string_stream ss(request);
    message::DeviceNotificationToJson(ss, notification);
    return request;
}

void DeviceSession::CreateSession(SessionId session_id) {
    using namespace message;

    if(_session_id) {
        TAU_LOG_WARNING("Single session only");
        return;
    }

    if(auto connection = _connection.lock()) {
        _session_id = session_id;

        DeviceNotification notification{
            .stream_id = *_stream_id,
            .session_id = session_id,
        };
        ws::String message;
        etl::string_stream ss(message);
        message::DeviceNotificationToJson(ss, notification);
        connection->PostMessage(std::move(message));
    } else {
        TAU_LOG_WARNING("No connection");
    }
}

void DeviceSession::CloseSession(SessionId session_id) {
    using namespace message;

    if(!_session_id || (session_id != *_session_id)) {
        TAU_LOG_WARNING("Wrong session_id");
        return;
    }
    _session_id.reset();

    if(auto connection = _connection.lock()) {
        DeviceNotification notification{
            .stream_id = *_stream_id,
            .session_id = session_id,
            .session_state = SessionState::kClosed
        };
        ws::String message;
        etl::string_stream ss(message);
        message::DeviceNotificationToJson(ss, notification);
        connection->PostMessage(std::move(message));
    } else {
        TAU_LOG_WARNING("No connection");
    }
}

void DeviceSession::SetSdpAnswer(message::Payload&& payload) {
    if(!_stream_id) {
        TAU_LOG_WARNING("No stream id");
        return;
    }
    if(!_session_id) {
        TAU_LOG_WARNING("No session id");
        return;
    }

    if(auto connection = _connection.lock()) {
        message::DeviceNotification notification{
            .stream_id = *_stream_id,
            .session_id = *_session_id,
            .session_state = SessionState::kSdpAnswered,
            .payload = std::move(payload)
        };
        ws::String message;
        etl::string_stream ss(message);
        message::DeviceNotificationToJson(ss, notification);
        connection->PostMessage(std::move(message));
    } else {
        TAU_LOG_WARNING("No connection");
    }
}

void DeviceSession::SetIceCandidates(message::Payload&& payload) {
    if(!_stream_id) {
        TAU_LOG_WARNING("No stream id");
        return;
    }
    if(!_session_id) {
        TAU_LOG_WARNING("No session id");
        return;
    }

    if(auto connection = _connection.lock()) {
        message::DeviceNotification notification{
            .stream_id = *_stream_id,
            .session_id = *_session_id,
            .session_state = SessionState::kSdpAnswered,
            .payload = std::move(payload)
        };
        ws::String message;
        etl::string_stream ss(message);
        message::DeviceNotificationToJson(ss, notification);
        connection->PostMessage(std::move(message));
    } else {
        TAU_LOG_WARNING("No connection");
    }
}

bool DeviceSession::IsActive() const {
    return (_connection.lock() != nullptr);
}

bool DeviceSession::ProcessNotification(const message::DeviceNotification& notification) {
    if(!_stream_id) {
        _stream_id = notification.stream_id;
    } else if(_stream_id != notification.stream_id) {
        TAU_LOG_WARNING("Wrong stream_id: " << notification.stream_id);
        return false;
    }
    return true;
}

}
