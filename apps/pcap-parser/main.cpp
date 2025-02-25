#include "tau/rtp/Reader.h"
#include "tau/common/Log.h"
#include <pcap.h>

constexpr auto kEthernetHeaderSize = 14;
constexpr auto kIpHeaderSize = 20;
constexpr auto kUdpHeaderSize = 8;

int main(int argc, char **argv) {
    InitLogging("pcap-parser", true);

    const uint8_t target_pt = 96;
    const std::string file = "/path/to/pcap/file.pcap";
    const size_t udp_payload_offset = kEthernetHeaderSize + kIpHeaderSize + kUdpHeaderSize;

    char error_buffer[PCAP_ERRBUF_SIZE];
    auto pcap = pcap_open_offline(file.c_str(), error_buffer);
    if(!pcap) {
        LOG_WARNING << "pcap_open_offline error: " << error_buffer;
        return -1;
    }

    size_t packet_count = 0;
    while(pcap) {
        struct pcap_pkthdr* header;
        const uint8_t* data;
        if(pcap_next_ex(pcap, &header, &data) < 0) {
            break;
        }
        LOG_TRACE << "Packet #" << (++packet_count) << ", " << "size: " << header->caplen << " bytes, " << "epoch: " << header->ts.tv_sec << "." << header->ts.tv_usec;

        const BufferViewConst packet_view{.ptr = data + udp_payload_offset, .size = header->caplen - udp_payload_offset};
        if(rtp::Reader::Validate(packet_view)) {
            rtp::Reader reader(packet_view);
            if(target_pt == reader.Pt()) {
                auto payload = reader.Payload();
                LOG_INFO << "[rtp] pt: " << (size_t)reader.Pt() << ", ssrc: " << reader.Ssrc() << ", ts: " << reader.Ts() << ", sn: " << reader.Sn() << (reader.Marker() ? "*" : "") << ", payload: " << payload.size;
            }
        }
    }
    pcap_close(pcap);
    LOG_INFO << "Done";
    return 0;
}
