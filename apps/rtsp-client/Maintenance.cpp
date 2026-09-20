#include "apps/rtsp-client/Maintenance.h"
#include "tau/rtsp/RequestWriter.h"
#include "tau/rtsp/ResponseReader.h"
#include "tau/common/String.h"
#include "tau/common/Exception.h"

namespace tau::rtsp {

Maintenance::Maintenance(Clock& clock, Connection& connection, size_t& cseq, std::weak_ptr<Session> session)
    : _clock(clock)
    , _connection(connection)
    , _cseq(cseq)
    , _session(std::move(session))
{}

void Maintenance::Start(etl::string_view uri, etl::string_view session_id, Timepoint timeout) {
    if((uri.size() > _uri.capacity()) || (session_id.size() > _session_id.capacity()) || (timeout == 0)) {
        TAU_EXCEPTION(std::invalid_argument, "Invalid options");
    }

    _started = true;
    _uri = uri;
    _session_id = session_id;
    _interval = timeout / 2;
    _next_keepalive = _clock.Now() + _interval;
}

void Maintenance::Stop() {
    _started = false;
    _response.reset();
}

void Maintenance::Process() {
    if(!_started) { return; }
    const auto session = _session.lock();
    if(!session) {
        Stop();
        return;
    }

    session->Process();
    if(_response && (_response->wait_for(std::chrono::seconds(0)) == std::future_status::ready)) {
        const auto message = _response->get();
        _response.reset();
        const auto response = ResponseReader::Read(message.data);
        if(!response || (response->status_code < 200) || (response->status_code >= 300)) {
            TAU_EXCEPTION(std::runtime_error, "RTSP keepalive rejected");
        }
    }

    if(!_response && (_clock.Now() >= _next_keepalive)) {
        const auto cseq = ToString<16>(++_cseq);
        StreamReader::String text;
        RequestWriter::Write(Request{
            .uri = _uri,
            .method = Method::kOptions,
            .headers = {
                {HeaderName::kCSeq, cseq},
                {HeaderName::kSession, _session_id}}
        }, text);
        _response.emplace(_connection.SendRequest(std::move(text), cseq));
        _next_keepalive = _clock.Now() + _interval;
    }
}

}
