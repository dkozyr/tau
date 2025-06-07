#include "apps/pcap-parser/Pipeline.h"
#include "tau/memory/SystemAllocator.h"
#include "tau/common/Log.h"
#include <boost/program_options.hpp>
#include <pcap.h>

using namespace tau;

constexpr auto kEthernetHeaderSize = 14;
constexpr auto kIpHeaderSize = 20;
constexpr auto kUdpHeaderSize = 8;

int main(int argc, char** argv) {
    InitLogging("pcap-parser", true);

    std::string pcap_file = std::string{PROJECT_SOURCE_DIR} + "/data/pcap/wilson.pcap";
    auto processing_type = static_cast<size_t>(Pipeline::Type::kH265);
    size_t pt = 104;
    std::string output_file = "raw.h265";

    namespace po = boost::program_options;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("pcap_file", po::value<std::string>(&pcap_file)->default_value(pcap_file), "pcap file path")
        ("type", po::value<size_t>(&processing_type)->default_value(2), "processing type: 0 - rtp only, 1 - h264 stream, 2 - h265 stream")
        ("pt", po::value<size_t>(&pt)->default_value(pt), "target payload type (pt)")
        ("output_file", po::value<std::string>(&output_file)->default_value(output_file), "output file path (not used for \"rtp only\" case)")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if(vm.count("help")) {
        TAU_LOG_INFO(desc);
        return 1;
    }

    char error_buffer[PCAP_ERRBUF_SIZE];
    auto pcap = pcap_open_offline(pcap_file.c_str(), error_buffer);
    if(!pcap) {
        TAU_LOG_WARNING("pcap_open_offline error: " << error_buffer);
        return -1;
    }

    Pipeline pipeline(Pipeline::Options{
        .type = static_cast<Pipeline::Type>(processing_type),
        .pt = static_cast<uint8_t>(pt),
        .output_file = output_file
    });

    size_t packet_count = 0;
    const size_t udp_payload_offset = kEthernetHeaderSize + kIpHeaderSize + kUdpHeaderSize;
    while(pcap) {
        struct pcap_pkthdr* header;
        const uint8_t* data;
        if(pcap_next_ex(pcap, &header, &data) < 0) {
            break;
        }
        TAU_LOG_TRACE("Packet #" << (++packet_count) << ", " << "size: " << header->caplen << " bytes, " << "epoch: " << header->ts.tv_sec << "." << header->ts.tv_usec);

        const BufferViewConst packet_view{
            .ptr = data + udp_payload_offset,
            .size = header->caplen - udp_payload_offset
        };
        auto packet = Buffer::Create(g_system_allocator, packet_view);
        pipeline.Process(std::move(packet));
    }
    pcap_close(pcap);
    TAU_LOG_INFO("Done");
    return 0;
}
