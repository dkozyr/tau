#include "tau/rtsp/Connection.h"
#include "tau/rtsp/ConnectionState.h"
#include "tau/common/Exception.h"

namespace tau::rtsp {

Connection::Connection(Executor executor, Options options) {
    if(options.timeout.count() <= 0) {
        TAU_EXCEPTION(std::invalid_argument, "Invalid RTSP timeout");
    }
    _state = std::make_shared<ConnectionState>(executor, std::move(options));
}

Connection::~Connection() {
    Close();
}

std::future<StreamReader::Message> Connection::SendRequest(StreamReader::String request, etl::string_view cseq) {
    return _state->SendRequest(std::move(request), cseq);
}

std::future<void> Connection::SendPacket(uint8_t channel, StreamReader::String packet) {
    return _state->SendPacket(channel, std::move(packet));
}

void Connection::SetPacketCallback(PacketCallback callback) {
    _state->SetPacketCallback(std::move(callback));
}

void Connection::SetCloseCallback(std::function<void()> callback) {
    _state->SetCloseCallback(std::move(callback));
}

void Connection::AsyncClose(std::function<void()> completion) {
    _state->AsyncClose(std::move(completion));
}

void Connection::Close() {
    _state->Close();
}

}
