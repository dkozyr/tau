#include "tau/video/h264/Avc1NaluProcessor.h"

namespace h264 {

Avc1NaluProcessor::Avc1NaluProcessor(Options&& options)
    : _sps(std::move(options.sps))
    , _pps(std::move(options.pps))
{}

void Avc1NaluProcessor::Push(Buffer&& nal_unit) {
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

void Avc1NaluProcessor::DropUntilKeyFrame() {
    _drop_until_key_frame = true;
}

void Avc1NaluProcessor::ProcessSpsPps() {
    if(_sps && _pps) {
        _callback(std::move(*_sps));
        _callback(std::move(*_pps));
        _sps.reset();
        _pps.reset();
        _sps_pps_processed = true;
    }
}

}
