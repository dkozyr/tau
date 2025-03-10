#include "tau/rtp-session/Session.h"
#include "tau/rtp/Reader.h"
#include "tau/rtp/Sn.h"
#include "tau/rtcp/Reader.h"
#include "tau/rtcp/Writer.h"
#include "tau/rtcp/SrReader.h"
#include "tau/rtcp/SrWriter.h"
#include "tau/rtcp/RrReader.h"
#include "tau/rtcp/RrWriter.h"
#include "tau/common/Ntp.h"

#include "tau/common/Log.h" //TODO: remove it

namespace rtp::session {

Session::Session(Dependencies&& deps, Options&& options)
    : _deps(std::move(deps))
    , _options(std::move(options))
    , _send_buffer(_options.send_buffer_size)
    , _recv_buffer(_options.recv_buffer_size)
    , _last_outgoing_rtcp(_deps.media_clock.Now())
    , _last_outgoing_rtcp_sr(_deps.media_clock.Now()) {
    _send_buffer.SetCallback([this](Buffer&& rtp_packet) { _send_rtp_callback(std::move(rtp_packet)); });
    _recv_buffer.SetCallback([this](Buffer&& rtp_packet) { _recv_rtp_callback(std::move(rtp_packet)); });
}

void Session::SendRtp(Buffer&& rtp_packet) {
    ProcessRtcpSr(rtp_packet);
    ProcessRtcp();
    Reader reader(ToConst(rtp_packet.GetView()));
    _send_buffer.Push(std::move(rtp_packet), reader.Sn());
}

void Session::Recv(Buffer&& packet) {
    if(rtcp::IsRtcp(ToConst(packet.GetView()))) {
        RecvRtcp(std::move(packet));
    } else {
        RecvRtp(std::move(packet));
    }
}

void Session::RecvRtp(Buffer&& rtp_packet) {
    ProcessRtcp();
    const auto view = ToConst(rtp_packet.GetView());
    if(!Reader::Validate(view)) {
        //TODO: stats.discarded++;
        return;
    }
    Reader reader(view);
    if(_recv_ctx) {
        if((_recv_ctx->pt != reader.Pt()) || (_recv_ctx->ssrc != reader.Ssrc())) {
            //TODO: stats.discarded++;
            return;
        }
    } else {
        _recv_ctx.emplace(RecvContext{
            .pt = reader.Pt(),
            .ssrc = reader.Ssrc(),
            .sn_first = reader.Sn(),
            .sn_last = reader.Sn(),
            .sn_cycles = 0,
            .jitter = Jitter(_options.rate, reader.Ts(), _deps.media_clock.Now()),
            .ts_converter = TsConverter(TsConverter::Options{
                .rate = _options.rate,
                .ts_base = reader.Ts()
            }),
        });
    }
    ProcessSn(reader.Sn());
    ProcessTs(rtp_packet, reader.Ts());
    _recv_buffer.Push(std::move(rtp_packet), reader.Sn());
}

void Session::RecvRtcp(Buffer&& rtcp_packet) {
    const auto view = ToConst(rtcp_packet.GetView());
    if(!rtcp::Reader::Validate(view)) {
        //TODO: stats.discarded++;
        return;
    }

    rtcp::Reader::ForEachReport(view, [this](rtcp::Type type, const BufferViewConst& report) {
        switch(type) {
            case rtcp::Type::kSr:
                ProcessIncomingRtcpSr(report);
                break;
            case rtcp::Type::kRr:
                ProcessIncomingRtcpRr(report);
                break;
            default:
                break;
        }
        return true;
    });
}

float Session::GetLossRate() const {
    const auto fraction_lost = rtcp::GetFractionLost(_last_packet_lost_word_from_rr);
    return static_cast<float>(fraction_lost) / 256.0f;
}

int32_t Session::GetLostPackets() const {
    return rtcp::GetCumulativePacketLost(_last_packet_lost_word_from_rr);
}

void Session::ProcessSn(uint16_t sn) {
    const auto sn_delta = SnDelta(sn, _recv_ctx->sn_last);
    if(sn_delta > 4096) { //TODO: name constant
        //TODO: reset states
        return;
    }

    if(SnLesser(_recv_ctx->sn_last, sn)) {
        if(_recv_ctx->sn_last > sn) {
            _recv_ctx->sn_cycles++;
        }
        _recv_ctx->sn_last = sn;
    }
}

void Session::ProcessTs(Buffer& rtp_packet, uint32_t rtp_ts) {
    rtp_packet.GetInfo().tp = _recv_ctx->ts_converter.FromTs(rtp_ts);
    _recv_ctx->jitter.Update(rtp_ts, _deps.media_clock.Now());
}

void Session::ProcessRtcp() {
    const auto now = _deps.media_clock.Now();
    if(now < _last_outgoing_rtcp + kSec) {
        return;
    }
    _last_outgoing_rtcp = now;

    rtcp::RrBlocks rr_blocks;
    if(_recv_ctx) {
        const auto& ctx = *_recv_ctx;
        rr_blocks.push_back(rtcp::RrBlock{
            .ssrc = ctx.ssrc,
            .packet_lost_word = rtcp::BuildPacketLostWord(0, 0), //TODO: fix it
            .ext_highest_sn = (static_cast<uint32_t>(ctx.sn_cycles) << 16) | ctx.sn_last,
            .jitter = ctx.jitter.Get(),
            .lsr = _lsr,
            .dlsr = ntp32::ToNtp(_last_incoming_rtcp_sr ? (_deps.media_clock.Now() - _last_incoming_rtcp_sr) : 0)
        });
    }

    auto packet = Buffer::Create(_deps.allocator, Buffer::Info{.tp = now});
    rtcp::Writer writer(packet.GetViewWithCapacity());
    if(_sr_info.ntp) {
        if(!rtcp::SrWriter::Write(writer, _options.sender_ssrc, _sr_info, rr_blocks)) {
            return;
        }
    } else {
        if(!rtcp::RrWriter::Write(writer, _options.sender_ssrc, rr_blocks)) {
            return;
        }
    }
    packet.SetSize(writer.GetSize());
    _send_rtcp_callback(std::move(packet));
}

void Session::ProcessRtcpSr(const Buffer& rtp_packet) {
    const auto now = _deps.media_clock.Now();
    if(now < _last_outgoing_rtcp_sr + kSec) {
        return;
    }
    _last_outgoing_rtcp_sr = now;

    const Reader reader(rtp_packet.GetView());
    const auto tp_delta = now - rtp_packet.GetInfo().tp;
    const auto ts_now = reader.Ts() + static_cast<uint32_t>(_options.rate * tp_delta / kSec);

    const auto& sender_stats = _send_buffer.GetStats();
    _sr_info.ntp = ToNtp(_deps.system_clock.Now());
    _sr_info.ts = ts_now;
    _sr_info.packet_count = sender_stats.packets;
    _sr_info.octet_count = sender_stats.bytes;
}

void Session::ProcessIncomingRtcpSr(const BufferViewConst& report) {
    if(_recv_ctx && (_recv_ctx->ssrc == rtcp::SrReader::GetSenderSsrc(report))) {
        const auto sr_info = rtcp::SrReader::GetSrInfo(report);
        _last_incoming_rtcp_sr = _deps.media_clock.Now();
        _lsr = NtpToNtp32(sr_info.ntp);
    }
}

void Session::ProcessIncomingRtcpRr(const BufferViewConst& report) {
    const auto rr_blocks = rtcp::RrReader::GetBlocks(report);
    for(auto& block : rr_blocks) {
        if(block.ssrc == _options.sender_ssrc) {
            _last_packet_lost_word_from_rr = block.packet_lost_word;

            const auto now_ntp32 = NtpToNtp32(ToNtp(_deps.system_clock.Now()));
            const auto rtt_ntp32 = now_ntp32 - block.lsr - block.dlsr;
            _rtt = ntp32::FromNtp(rtt_ntp32);
            break;
        }
    }
}

}
