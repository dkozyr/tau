#include "tau/video/h264/AvcNaluProcessor.h"

namespace tau::h264 {

AvcNaluProcessor::AvcNaluProcessor(Options&& options)
    : _type(options.type)
    , _sps(std::move(options.sps))
    , _pps(std::move(options.pps))
{}

void AvcNaluProcessor::Push(Buffer&& nal_unit) {
    const auto header = reinterpret_cast<const NaluHeader*>(&nal_unit.GetView().ptr[0]);
    switch(header->type) {
        case NaluType::kSps:
            if(!_sps_pps_processed) {
                _sps.emplace(std::move(nal_unit));
                ProcessSpsPps();
            }
            break;

        case NaluType::kPps:
            if(!_sps_pps_processed) {
                _pps.emplace(std::move(nal_unit));
                ProcessSpsPps();
            }
            break;

        case NaluType::kIdr:
            ProcessSpsPps();
            if(_sps_pps_processed) {
                if(_type == Type::kAvc3) {
                    _sps_pps_processed = false;
                }
                _drop_until_key_frame = false;
                _callback(std::move(nal_unit));
            }
            break;

        default:
            if(!_drop_until_key_frame) {
                _callback(std::move(nal_unit));
            }
            break;
    }
}

void AvcNaluProcessor::DropUntilKeyFrame() {
    _drop_until_key_frame = true;
}

void AvcNaluProcessor::ProcessSpsPps() {
    if(_sps && _pps) {
        _callback(std::move(*_sps));
        _callback(std::move(*_pps));
        _sps.reset();
        _pps.reset();
        _sps_pps_processed = true;
    }
}

}
