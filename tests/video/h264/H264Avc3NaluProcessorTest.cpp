#include "tau/video/h264/AvcNaluProcessor.h"
#include "tests/lib/Common.h"
#include "tests/lib/H264Utils.h"

namespace tau::h264 {

class H264Avc3NaluProcessorTest : public ::testing::Test {
public:
    H264Avc3NaluProcessorTest() {
        _avc3_nalu_processor.emplace(AvcNaluProcessor::Options{
            .type = AvcNaluProcessor::Type::kAvc3
        });
        InitCallback();
    }

    void InitCallback() {
        _avc3_nalu_processor->SetCallback([this](Buffer&& nal_unit) {
            _processed_nalu_units.push_back(std::move(nal_unit));
        });
    }

protected:
    void AssertProcessedNalUnits(const std::vector<NaluType>& target_nalu_types) {
        ASSERT_EQ(target_nalu_types.size(), _processed_nalu_units.size());
        for(size_t i = 0; i < target_nalu_types.size(); ++i) {
            const auto header = reinterpret_cast<const NaluHeader*>(&_processed_nalu_units[i].GetView().ptr[0]);
            ASSERT_EQ(target_nalu_types[i], header->type);
        }
    }

protected:
    std::optional<AvcNaluProcessor> _avc3_nalu_processor;
    std::vector<Buffer> _processed_nalu_units;
};

TEST_F(H264Avc3NaluProcessorTest, Basic) {
    _avc3_nalu_processor->Push(CreateNalu(kSps));
    _avc3_nalu_processor->Push(CreateNalu(kPps));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps}));

    _avc3_nalu_processor->Push(CreateNalu(kSei));
    _avc3_nalu_processor->Push(CreateNalu(kIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr}));

    _avc3_nalu_processor->Push(CreateNalu(kNonIdr));
    _avc3_nalu_processor->Push(CreateNalu(kNonIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr, kNonIdr, kNonIdr}));

    _avc3_nalu_processor->Push(CreateNalu(kIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr, kNonIdr, kNonIdr}));

    _avc3_nalu_processor->Push(CreateNalu(kSps));
    _avc3_nalu_processor->Push(CreateNalu(kPps));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr, kNonIdr, kNonIdr, kSps, kPps}));

    _avc3_nalu_processor->Push(CreateNalu(kIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr, kNonIdr, kNonIdr, kSps, kPps, kIdr}));
}

TEST_F(H264Avc3NaluProcessorTest, Options) {
    _avc3_nalu_processor.emplace(AvcNaluProcessor::Options{
        .sps = CreateNalu(kSps),
        .pps = CreateNalu(kPps)
    });
    InitCallback();
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({}));

    _avc3_nalu_processor->Push(CreateNalu(kIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr}));

    _avc3_nalu_processor->Push(CreateNalu(kNonIdr));
    _avc3_nalu_processor->Push(CreateNalu(kNonIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr, kNonIdr, kNonIdr}));

    _avc3_nalu_processor->Push(CreateNalu(kSps));
    _avc3_nalu_processor->Push(CreateNalu(kPps));
    _avc3_nalu_processor->Push(CreateNalu(kIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr, kNonIdr, kNonIdr, kIdr}));
}

TEST_F(H264Avc3NaluProcessorTest, LostSps) {
    _avc3_nalu_processor->Push(CreateNalu(kSps));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({}));

    _avc3_nalu_processor->Push(CreateNalu(kIdr));
    _avc3_nalu_processor->Push(CreateNalu(kNonIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({}));

    _avc3_nalu_processor->Push(CreateNalu(kPps));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps}));
    _avc3_nalu_processor->Push(CreateNalu(kIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr}));
}

TEST_F(H264Avc3NaluProcessorTest, LostPps) {
    _avc3_nalu_processor->Push(CreateNalu(kPps));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({}));

    _avc3_nalu_processor->Push(CreateNalu(kIdr));
    _avc3_nalu_processor->Push(CreateNalu(kNonIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({}));

    _avc3_nalu_processor->Push(CreateNalu(kSps));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps}));
    _avc3_nalu_processor->Push(CreateNalu(kIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr}));
}

TEST_F(H264Avc3NaluProcessorTest, LostIdr) {
    _avc3_nalu_processor->Push(CreateNalu(kSps));
    _avc3_nalu_processor->Push(CreateNalu(kPps));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps}));

    _avc3_nalu_processor->Push(CreateNalu(kNonIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps}));

    _avc3_nalu_processor->Push(CreateNalu(kIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr}));
}

TEST_F(H264Avc3NaluProcessorTest, DropUntilKeyFrame) {
    _avc3_nalu_processor->Push(CreateNalu(kSps));
    _avc3_nalu_processor->Push(CreateNalu(kPps));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps}));

    _avc3_nalu_processor->Push(CreateNalu(kIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr}));

    _avc3_nalu_processor->Push(CreateNalu(kNonIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr, kNonIdr}));

    _avc3_nalu_processor->DropUntilKeyFrame();
    _avc3_nalu_processor->Push(CreateNalu(kNonIdr));
    _avc3_nalu_processor->Push(CreateNalu(kNonIdr));
    _avc3_nalu_processor->Push(CreateNalu(kSps));
    _avc3_nalu_processor->Push(CreateNalu(kPps));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr, kNonIdr, kSps, kPps}));

    _avc3_nalu_processor->Push(CreateNalu(kIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr, kNonIdr, kSps, kPps, kIdr}));
}

}
