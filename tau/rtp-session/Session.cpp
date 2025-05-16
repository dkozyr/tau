#include "tau/rtp-session/Session.h"
#include "tau/rtp/Reader.h"
#include "tau/rtp/Sn.h"
#include "tau/rtcp/Reader.h"
#include "tau/rtcp/Writer.h"
#include "tau/rtcp/SrReader.h"
#include "tau/rtcp/SrWriter.h"
#include "tau/rtcp/RrReader.h"
#include "tau/rtcp/RrWriter.h"
#include "tau/rtcp/SdesWriter.h"
#include "tau/rtcp/FirReader.h"
#include "tau/rtcp/FirWriter.h"
#include "tau/rtcp/PliReader.h"
#include "tau/rtcp/PliWriter.h"
#include "tau/common/Ntp.h"

namespace tau::rtp::session {

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
    const auto view = ToConst(rtp_packet.GetView());
    if(!Reader::Validate(view)) {
        _stats.incoming.discarded++;
        return;
    }
    _stats.incoming.rtp++;

    Reader reader(view);
    if(_recv_ctx) {
        if((_recv_ctx->pt != reader.Pt()) || (_rr_block.ssrc != reader.Ssrc())) {
            _stats.incoming.discarded++;
            return;
        }
    } else {
        _recv_ctx.emplace(RecvContext{
            .pt = reader.Pt(),
            .sn_first = reader.Sn(),
            .sn_last = reader.Sn(),
            .sn_cycles = 0,
            .jitter = Jitter(_options.rate, reader.Ts(), _deps.media_clock.Now()),
            .ts_converter = TsConverter(TsConverter::Options{
                .rate = _options.rate,
                .ts_base = reader.Ts()
            }),
        });
        _rr_block.ssrc = reader.Ssrc();
        _rr_block.ext_highest_sn = SnBackward(reader.Sn(), 1);
    }
    ProcessSn(reader.Sn());
    ProcessTs(rtp_packet, reader.Ts());
    _recv_buffer.Push(std::move(rtp_packet), reader.Sn());
    ProcessRtcp();
}

void Session::RecvRtcp(Buffer&& rtcp_packet) {
    const auto view = ToConst(rtcp_packet.GetView());
    if(!rtcp::Reader::Validate(view)) {
        _stats.incoming.discarded++;
        return;
    }

    rtcp::Reader::ForEachReport(view, [this](rtcp::Type type, const BufferViewConst& report) {
        switch(type) {
            case rtcp::Type::kSr:   ProcessIncomingRtcpSr(report); break;
            case rtcp::Type::kRr:   ProcessIncomingRtcpRr(report); break;
            case rtcp::Type::kPsfb: ProcessIncomingRtcpPsfb(report); break;
            default:
                break;
        }
        return true;
    });
}

void Session::PushEvent(Event&& event) {
    if(!_recv_ctx) {
        return;
    }
    auto rtcp_packet = Buffer::Create(_deps.allocator, Buffer::Info{.tp = _deps.media_clock.Now()});
    rtcp::Writer writer(rtcp_packet.GetViewWithCapacity());
    UpdateRrBlock();
    if(!rtcp::RrWriter::Write(writer, _options.sender_ssrc, {_rr_block})) {
        return;
    }
    switch(event) {
        case Event::kFir:
            if(!rtcp::FirWriter::Write(writer, _options.sender_ssrc, _rr_block.ssrc, 1)) { //TODO: fix FIR sn
                return;
            }
            break;
        case Event::kPli:
            if(!rtcp::PliWriter::Write(writer, _options.sender_ssrc, _rr_block.ssrc)) {
                return;
            }
            break;
    }
    rtcp_packet.SetSize(writer.GetSize());
    _send_rtcp_callback(std::move(rtcp_packet));
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
        UpdateRrBlock();
        rr_blocks.push_back(_rr_block);
    }

    auto packet = Buffer::Create(_deps.allocator, Buffer::Info{.tp = now});
    rtcp::Writer writer(packet.GetViewWithCapacity());
    if(_sr_info.ntp) {
        if(!rtcp::SrWriter::Write(writer, _options.sender_ssrc, _sr_info, rr_blocks)) {
            return;
        }
        if(!_options.cname.empty()) {
            if(!rtcp::SdesWriter::Write(writer, {
                    rtcp::SdesChunk{
                        .ssrc = _options.sender_ssrc,
                        .items = {
                            rtcp::SdesItem{.type = rtcp::SdesType::kCname, .data = _options.cname},
                        }
                    }
                })) {
                return;
            }
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

void Session::UpdateRrBlock() {
    auto& ctx = *_recv_ctx;
    const auto ext_highest_sn = (static_cast<uint32_t>(ctx.sn_cycles) << 16) | ctx.sn_last;
    const auto received_packets = _recv_buffer.GetStats().packets;
    const auto expected_packets = 1u + ext_highest_sn - static_cast<uint32_t>(ctx.sn_first);

    const auto expected_delta = ext_highest_sn - _rr_block.ext_highest_sn;
    const auto received_delta = received_packets - ctx.received_packets;
    ctx.received_packets = received_packets;

    const auto fraction_lost = rtcp::BuildFractionLost(received_delta, expected_delta);
    const auto cumulative_packet_lost = rtcp::BuildCumulativePacketLost(ctx.received_packets, expected_packets);
    _rr_block.packet_lost_word = rtcp::BuildPacketLostWord(fraction_lost, cumulative_packet_lost);
    _rr_block.ext_highest_sn = ext_highest_sn;
    _rr_block.jitter = ctx.jitter.Get();
    _rr_block.dlsr = ntp32::ToNtp(_last_incoming_rtcp_sr ? (_deps.media_clock.Now() - _last_incoming_rtcp_sr) : 0);

    _stats.incoming.jitter = _rr_block.jitter;
    _stats.incoming.loss_rate = static_cast<float>(cumulative_packet_lost) / expected_packets;
    _stats.incoming.lost_packets = cumulative_packet_lost;
}

void Session::ProcessIncomingRtcpSr(const BufferViewConst& report) {
    if(_recv_ctx && (_rr_block.ssrc == rtcp::SrReader::GetSenderSsrc(report))) {
        const auto sr_info = rtcp::SrReader::GetSrInfo(report);
        _last_incoming_rtcp_sr = _deps.media_clock.Now();
        _rr_block.lsr = NtpToNtp32(sr_info.ntp);
    }
}

void Session::ProcessIncomingRtcpRr(const BufferViewConst& report) {
    const auto rr_blocks = rtcp::RrReader::GetBlocks(report);
    for(auto& block : rr_blocks) {
        if(block.ssrc == _options.sender_ssrc) {
            const auto now_ntp32 = NtpToNtp32(ToNtp(_deps.system_clock.Now()));
            const auto rtt_ntp32 = now_ntp32 - block.lsr - block.dlsr;
            _stats.rtt = ntp32::FromNtp(rtt_ntp32);
            _stats.outgoing.loss_rate = static_cast<float>(rtcp::GetFractionLost(block.packet_lost_word)) / 256.0f;
            _stats.outgoing.lost_packets = rtcp::GetCumulativePacketLost(block.packet_lost_word);
            break;
        }
    }
}

void Session::ProcessIncomingRtcpPsfb(const BufferViewConst& report) {
    const auto fmt = rtcp::GetRc(report.ptr[0]);
    switch(fmt) {
        case rtcp::PsfbType::kPli:
            if(rtcp::PliReader::GetMediaSsrc(report) == _options.sender_ssrc) {
                _event_callback(Event::kPli);
            }
            break;
        case rtcp::PsfbType::kFir:
            if(rtcp::FirReader::GetMediaSsrc(report) == _options.sender_ssrc) {
                _event_callback(Event::kFir);
            }
            break;
        default:
            break;
    }
}

}
