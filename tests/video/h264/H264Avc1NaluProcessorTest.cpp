#include "tau/video/h264/Avc1NaluProcessor.h"
#include "tests/Common.h"

namespace h264 {

class H264Avc1NaluProcessorTest : public ::testing::Test {
public:
    H264Avc1NaluProcessorTest() {
        _avc1_nalu_processor.emplace(Avc1NaluProcessor::Options{});
        InitCallback();
    }

    void InitCallback() {
        _avc1_nalu_processor->SetCallback([this](Buffer&& nal_unit) {
            _processed_nalu_units.push_back(std::move(nal_unit));
        });
    }

protected:
    //TODO: move to test Utils file
    static Buffer CreateNalu(NaluType type, size_t size = 256) {
        auto nalu = Buffer::Create(g_system_allocator, size);
        auto view = nalu.GetViewWithCapacity();
        view.ptr[0] = CreateNalUnitHeader(type, 0b11);
        for(size_t i = 1; i < size; ++i) {
            view.ptr[i] = i;
        }
        nalu.SetSize(size);
        return nalu;
    }

    void AssertProcessedNalUnits(const std::vector<NaluType>& target_nalu_types) {
        ASSERT_EQ(target_nalu_types.size(), _processed_nalu_units.size());
        for(size_t i = 0; i < target_nalu_types.size(); ++i) {
            const auto header = reinterpret_cast<const NaluHeader*>(&_processed_nalu_units[i].GetView().ptr[0]);
            ASSERT_EQ(target_nalu_types[i], header->type);
        }
    }

protected:
    std::optional<Avc1NaluProcessor> _avc1_nalu_processor;
    std::vector<Buffer> _processed_nalu_units;
};

TEST_F(H264Avc1NaluProcessorTest, Basic) {
    _avc1_nalu_processor->Push(CreateNalu(kSps));
    _avc1_nalu_processor->Push(CreateNalu(kPps));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps}));

    _avc1_nalu_processor->Push(CreateNalu(kSei));
    _avc1_nalu_processor->Push(CreateNalu(kIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr}));

    _avc1_nalu_processor->Push(CreateNalu(kNonIdr));
    _avc1_nalu_processor->Push(CreateNalu(kNonIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr, kNonIdr, kNonIdr}));

    _avc1_nalu_processor->Push(CreateNalu(kIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr, kNonIdr, kNonIdr, kIdr}));

    _avc1_nalu_processor->Push(CreateNalu(kSps));
    _avc1_nalu_processor->Push(CreateNalu(kPps));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr, kNonIdr, kNonIdr, kIdr}));
}

TEST_F(H264Avc1NaluProcessorTest, LostSps) {
    _avc1_nalu_processor->Push(CreateNalu(kSps));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({}));

    _avc1_nalu_processor->Push(CreateNalu(kIdr));
    _avc1_nalu_processor->Push(CreateNalu(kNonIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({}));

    _avc1_nalu_processor->Push(CreateNalu(kPps));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps}));
    _avc1_nalu_processor->Push(CreateNalu(kIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr}));
}

TEST_F(H264Avc1NaluProcessorTest, LostPps) {
    _avc1_nalu_processor->Push(CreateNalu(kPps));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({}));

    _avc1_nalu_processor->Push(CreateNalu(kIdr));
    _avc1_nalu_processor->Push(CreateNalu(kNonIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({}));

    _avc1_nalu_processor->Push(CreateNalu(kSps));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps}));
    _avc1_nalu_processor->Push(CreateNalu(kIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr}));
}

TEST_F(H264Avc1NaluProcessorTest, LostIdr) {
    _avc1_nalu_processor->Push(CreateNalu(kSps));
    _avc1_nalu_processor->Push(CreateNalu(kPps));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps}));

    _avc1_nalu_processor->Push(CreateNalu(kNonIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps}));

    _avc1_nalu_processor->Push(CreateNalu(kIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr}));
}

TEST_F(H264Avc1NaluProcessorTest, DropUntilKeyFrame) {
    _avc1_nalu_processor->Push(CreateNalu(kSps));
    _avc1_nalu_processor->Push(CreateNalu(kPps));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps}));

    _avc1_nalu_processor->Push(CreateNalu(kIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr}));

    _avc1_nalu_processor->Push(CreateNalu(kNonIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr, kNonIdr}));

    _avc1_nalu_processor->DropUntilKeyFrame();
    _avc1_nalu_processor->Push(CreateNalu(kNonIdr));
    _avc1_nalu_processor->Push(CreateNalu(kNonIdr));
    _avc1_nalu_processor->Push(CreateNalu(kSps));
    _avc1_nalu_processor->Push(CreateNalu(kPps));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr, kNonIdr}));

    _avc1_nalu_processor->Push(CreateNalu(kIdr));
    ASSERT_NO_FATAL_FAILURE(AssertProcessedNalUnits({kSps, kPps, kIdr, kNonIdr, kIdr}));
}

}
