#include "tau/video/h264/AvcNaluProcessor.h"
#include "tests/lib/Common.h"
#include "tests/lib/NaluUtils.h"

namespace tau::h264 {

class H264Avc1NaluProcessorTest : public ::testing::Test {
public:
    H264Avc1NaluProcessorTest() {
        _avc1_nalu_processor.emplace(AvcNaluProcessor::Options{
            .type = AvcNaluProcessor::Type::kAvc1
        });
        InitCallback();
    }

    void InitCallback() {
        _avc1_nalu_processor->SetCallback([this](Buffer&& nal_unit) {
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
    std::optional<AvcNaluProcessor> _avc1_nalu_processor;
    std::vector<Buffer> _processed_nalu_units;
};

TEST_F(H264Avc1NaluProcessorTest, Basic) {
    _avc1_nalu_processor->Push(CreateH264Nalu(kSps));
    _avc1_nalu_processor->Push(CreateH264Nalu(kPps));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps}));

    _avc1_nalu_processor->Push(CreateH264Nalu(kSei));
    _avc1_nalu_processor->Push(CreateH264Nalu(kIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr}));

    _avc1_nalu_processor->Push(CreateH264Nalu(kNonIdr));
    _avc1_nalu_processor->Push(CreateH264Nalu(kNonIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr, kNonIdr, kNonIdr}));

    _avc1_nalu_processor->Push(CreateH264Nalu(kIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr, kNonIdr, kNonIdr, kIdr}));

    _avc1_nalu_processor->Push(CreateH264Nalu(kSps));
    _avc1_nalu_processor->Push(CreateH264Nalu(kPps));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr, kNonIdr, kNonIdr, kIdr}));
}

TEST_F(H264Avc1NaluProcessorTest, Options) {
    _avc1_nalu_processor.emplace(AvcNaluProcessor::Options{
        .sps = CreateH264Nalu(kSps),
        .pps = CreateH264Nalu(kPps)
    });
    InitCallback();
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({}));

    _avc1_nalu_processor->Push(CreateH264Nalu(kIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr}));

    _avc1_nalu_processor->Push(CreateH264Nalu(kNonIdr));
    _avc1_nalu_processor->Push(CreateH264Nalu(kNonIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr, kNonIdr, kNonIdr}));

    _avc1_nalu_processor->Push(CreateH264Nalu(kSps));
    _avc1_nalu_processor->Push(CreateH264Nalu(kPps));
    _avc1_nalu_processor->Push(CreateH264Nalu(kIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr, kNonIdr, kNonIdr, kIdr}));
}

TEST_F(H264Avc1NaluProcessorTest, LostSps) {
    _avc1_nalu_processor->Push(CreateH264Nalu(kSps));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({}));

    _avc1_nalu_processor->Push(CreateH264Nalu(kIdr));
    _avc1_nalu_processor->Push(CreateH264Nalu(kNonIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({}));

    _avc1_nalu_processor->Push(CreateH264Nalu(kPps));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps}));
    _avc1_nalu_processor->Push(CreateH264Nalu(kIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr}));
}

TEST_F(H264Avc1NaluProcessorTest, LostPps) {
    _avc1_nalu_processor->Push(CreateH264Nalu(kPps));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({}));

    _avc1_nalu_processor->Push(CreateH264Nalu(kIdr));
    _avc1_nalu_processor->Push(CreateH264Nalu(kNonIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({}));

    _avc1_nalu_processor->Push(CreateH264Nalu(kSps));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps}));
    _avc1_nalu_processor->Push(CreateH264Nalu(kIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr}));
}

TEST_F(H264Avc1NaluProcessorTest, LostIdr) {
    _avc1_nalu_processor->Push(CreateH264Nalu(kSps));
    _avc1_nalu_processor->Push(CreateH264Nalu(kPps));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps}));

    _avc1_nalu_processor->Push(CreateH264Nalu(kNonIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps}));

    _avc1_nalu_processor->Push(CreateH264Nalu(kIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr}));
}

TEST_F(H264Avc1NaluProcessorTest, DropUntilKeyFrame) {
    _avc1_nalu_processor->Push(CreateH264Nalu(kSps));
    _avc1_nalu_processor->Push(CreateH264Nalu(kPps));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps}));

    _avc1_nalu_processor->Push(CreateH264Nalu(kIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr}));

    _avc1_nalu_processor->Push(CreateH264Nalu(kNonIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr, kNonIdr}));

    _avc1_nalu_processor->DropUntilKeyFrame();
    _avc1_nalu_processor->Push(CreateH264Nalu(kNonIdr));
    _avc1_nalu_processor->Push(CreateH264Nalu(kNonIdr));
    _avc1_nalu_processor->Push(CreateH264Nalu(kSps));
    _avc1_nalu_processor->Push(CreateH264Nalu(kPps));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr, kNonIdr}));

    _avc1_nalu_processor->Push(CreateH264Nalu(kIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr, kNonIdr, kIdr}));
}

}
