#include "tau/rtcp/Reader.h"
#include "tau/rtcp/RrReader.h"
#include "tau/rtcp/SrReader.h"
#include "tau/rtcp/ByeReader.h"
#include "tau/rtcp/PliReader.h"
#include "tau/rtcp/FirReader.h"
#include "tau/rtcp/NackReader.h"
#include "tau/rtcp/SdesReader.h"
#include "tau/common/NetToHost.h"

namespace tau::rtcp {

bool Reader::ForEachReport(const BufferViewConst& view, ReportCallback callback) {
    auto begin = view.ptr;
    auto end = begin + view.size;
    while(begin + kHeaderSize <= end) {
        const auto type = static_cast<Type>(begin[1]);
        if((type < Type::kSr) || (type > Type::kXr)) {
            break;
        }
        const auto report_words_minus_one = Read16(begin + sizeof(uint16_t));
        const auto report_size = (1 + report_words_minus_one) * sizeof(uint32_t);
        if(begin + report_size > end) {
            break;
        }
        size_t padding_size = 0;
        if(HasPadding(begin[0])) {
            if(begin + report_size != end) {
                break;
            }
            padding_size = std::max<uint8_t>(1, begin[report_size - 1]);
            if(kHeaderSize + padding_size > report_size) {
                break;
            }
        }
        if(!callback(type, BufferViewConst{.ptr = begin, .size = report_size - padding_size})) {
            break;
        }
        begin += report_size;
    }
    return (begin == end);
}

bool Reader::Validate(const BufferViewConst& view) {
    return ForEachReport(view, [](Type type, const BufferViewConst& report) {
        switch(type) {
            case Type::kRr:   return RrReader::Validate(report);
            case Type::kSr:   return SrReader::Validate(report);
            case Type::kSdes: return SdesReader::Validate(report);
            case Type::kBye:  return ByeReader::Validate(report);
            case Type::kRtpfb: {
                const auto fmt = GetRc(report.ptr[0]);
                switch(fmt) {
                    case RtpfbType::kNack: return NackReader::Validate(report);
                    default:
                        break;
                }
                break;
            }
            case Type::kPsfb: {
                const auto fmt = GetRc(report.ptr[0]);
                switch(fmt) {
                    case PsfbType::kPli: return PliReader::Validate(report);
                    case PsfbType::kFir: return FirReader::Validate(report);
                    default:
                        break;
                }
                break;
            }
            default:
                break;
        }
        return true;
    });
}

}
