#include "apps/rtsp-client/Maintenance.h"
#include "tau/rtsp/RequestWriter.h"
#include "tau/rtsp/ResponseReader.h"
#include "tau/common/String.h"
#include "tau/common/Log.h"

namespace tau::rtsp {

Maintenance::Maintenance(Dependencies deps, Options options)
    : _clock(deps.clock)
    , _timer(deps.executor)
    , _connection(std::move(options.connection))
    , _session(std::move(options.session))
    , _cseq(std::move(options.cseq))
    , _uri(options.uri)
    , _session_id(options.session_id)
    , _interval(options.timeout / 2)
{}

void Maintenance::Start() {
    std::lock_guard lock{_mutex};
    if(!_started && !_stopped) {
        _started = true;
        _next_keepalive = _clock.Now() + _interval;
        Schedule();
    }
}

void Maintenance::Stop() {
    std::lock_guard lock{_mutex};
    StopInternal();
}

void Maintenance::StopInternal() {
    _stopped = true;
    _timer.cancel();
    _response.reset();
}

void Maintenance::Schedule() {
    _timer.expires_after(std::chrono::nanoseconds(100 * kMs));
    _timer.async_wait([weak = weak_from_this()](boost_ec ec) {
        if(auto self = weak.lock()) {
            self->OnTimer(ec);
        }
    });
}

void Maintenance::OnTimer(boost_ec ec) {
    auto [connection, session] = GetConnectionWithSession(ec);
    if(!connection || !session) {
        Stop();
        return;
    }

    try {
        session->Process();

        std::unique_lock lock{_mutex};
        if(_stopped) { return; }

        if(_response && (_response->wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)) {
            const auto message = _response->get();
            const auto response = ResponseReader::Read(message.data);
            if(!response || (response->status_code < 200) || (response->status_code >= 300)) {
                throw std::runtime_error("RTSP keepalive rejected");
            }
            _response.reset();
        }

        if(!_response && (_clock.Now() >= _next_keepalive)) {
            const auto cseq = ToString<16>(++_cseq);
            StreamReader::String text;
            RequestWriter::Write(Request{
                .uri = _uri,
                .method = Method::kOptions,
                .headers = {
                    {HeaderName::kCSeq, cseq},
                    {HeaderName::kSession, _session_id}
                }
            }, text);
            _response.emplace(connection->SendRequest(std::move(text), cseq));
            _next_keepalive = _clock.Now() + _interval;
        }
    } catch(const std::exception& exception) {
        Stop();
        TAU_LOG_WARNING("Exception: " << exception.what());
        connection->Close();
        return;
    }

    Schedule();
}

Maintenance::ConnectionWithSession Maintenance::GetConnectionWithSession(boost_ec ec) {
    std::unique_lock lock{_mutex};
    if(ec || _stopped) {
        return ConnectionWithSession{nullptr, nullptr};
    }

    return std::make_pair(_connection.lock(), _session.lock());
}

}
