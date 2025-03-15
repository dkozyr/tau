#include "tau/rtcp/Reader.h"
#include "tau/rtcp/Writer.h"
#include "tau/rtcp/SrReader.h"
#include "tau/rtcp/SrWriter.h"
#include "tau/rtcp/RrReader.h"
#include "tau/rtcp/RrWriter.h"
#include "tau/rtcp/ByeReader.h"
#include "tau/rtcp/ByeWriter.h"
#include "tau/rtcp/PliReader.h"
#include "tau/rtcp/PliWriter.h"
#include "tau/rtcp/FirReader.h"
#include "tau/rtcp/FirWriter.h"
#include "tau/rtcp/NackReader.h"
#include "tau/rtcp/NackWriter.h"
#include "tau/memory/Buffer.h"
#include "tests/Common.h"

namespace tau::rtcp {

class ReaderWriterTest : public ::testing::Test {
public:
    ReaderWriterTest()
        : _sender_ssrc(g_random.Int<uint32_t>())
        , _media_ssrc(g_random.Int<uint32_t>())
    {}

protected:
    SrInfo CreateSrInfo() {
        return SrInfo{
            .ntp          = g_random.Int<uint64_t>(),
            .ts           = g_random.Int<uint32_t>(),
            .packet_count = g_random.Int<uint32_t>(),
            .octet_count  = g_random.Int<uint32_t>()
        };
    }

    RrBlocks CreateRrBlocks(uint8_t blocks_count) {
        RrBlocks blocks;
        for(uint8_t i = 0; i < blocks_count; ++i) {
            blocks.push_back(CreateRrBlock());
        }
        return blocks;
    }

    RrBlock CreateRrBlock() {
        return RrBlock{
            .ssrc             = g_random.Int<uint32_t>(),
            .packet_lost_word = BuildPacketLostWord(g_random.Int<uint8_t>(), g_random.Int<int32_t>(kCumulativeLostMin, kCumulativeLostMax)),
            .ext_highest_sn   = g_random.Int<uint32_t>(),
            .jitter           = g_random.Int<uint32_t>(),
            .lsr              = g_random.Int<uint32_t>(),
            .dlsr             = g_random.Int<uint32_t>()
        };
    }

    NackSns CreateNackSns(size_t count) {
        NackSns sns;
        for(size_t i = 0; i < count; ++i) {
            sns.insert(g_random.Int<uint16_t>());
        }
        return sns;
    }

    std::vector<uint32_t> ByeSsrcs(size_t count) {
        std::vector<uint32_t> ssrcs;
        for(size_t i = 0; i < count; ++i) {
            ssrcs.push_back(g_random.Int<uint32_t>());
        }
        return ssrcs;
    }

    static void AssertRrBlocks(const RrBlocks& target, const RrBlocks& actual) {
        ASSERT_EQ(target.size(), actual.size());
        for(size_t i = 0; i < target.size(); ++i) {
            ASSERT_EQ(target[i], actual[i]);
        }
    }

protected:
    uint32_t _sender_ssrc;
    uint32_t _media_ssrc;
};

TEST_F(ReaderWriterTest, Sr) {
    auto packet = Buffer::Create(g_system_allocator, 1500);
    const auto info = CreateSrInfo();
    const auto rr_blocks = CreateRrBlocks(3);

    Writer writer(packet.GetViewWithCapacity());
    ASSERT_TRUE(SrWriter::Write(writer, _sender_ssrc, info, rr_blocks));
    packet.SetSize(writer.GetSize());

    const auto view = ToConst(packet.GetView());
    ASSERT_TRUE(Reader::Validate(view));
    ASSERT_EQ(_sender_ssrc, SrReader::GetSenderSsrc(view));
    ASSERT_EQ(info, SrReader::GetSrInfo(view));
    ASSERT_NO_FATAL_FAILURE(AssertRrBlocks(rr_blocks, SrReader::GetBlocks(view)));
}

TEST_F(ReaderWriterTest, Rr) {
    auto packet = Buffer::Create(g_system_allocator, 1500);
    const auto rr_blocks = CreateRrBlocks(5);

    Writer writer(packet.GetViewWithCapacity());
    ASSERT_TRUE(RrWriter::Write(writer, _sender_ssrc, rr_blocks));
    packet.SetSize(writer.GetSize());

    const auto view = ToConst(packet.GetView());
    ASSERT_TRUE(Reader::Validate(view));
    ASSERT_EQ(_sender_ssrc, RrReader::GetSenderSsrc(view));
    ASSERT_NO_FATAL_FAILURE(AssertRrBlocks(rr_blocks, RrReader::GetBlocks(view)));
}

TEST_F(ReaderWriterTest, Bye) {
    auto packet = Buffer::Create(g_system_allocator, 1500);
    const auto ssrcs = ByeSsrcs(3);

    Writer writer(packet.GetViewWithCapacity());
    ASSERT_TRUE(ByeWriter::Write(writer, ssrcs));
    packet.SetSize(writer.GetSize());

    const auto view = ToConst(packet.GetView());
    ASSERT_TRUE(Reader::Validate(view));
    ASSERT_EQ(ssrcs, ByeReader::GetSsrcs(view));
}

TEST_F(ReaderWriterTest, WrongBye) {
    auto packet = Buffer::Create(g_system_allocator, 1500);
    const auto ssrcs = ByeSsrcs(3);

    Writer writer(packet.GetViewWithCapacity());
    ASSERT_TRUE(ByeWriter::Write(writer, ssrcs));
    packet.SetSize(writer.GetSize());
    packet.GetView().ptr[0] = BuildHeader(ssrcs.size() + 1);

    const auto view = ToConst(packet.GetView());
    ASSERT_FALSE(Reader::Validate(view));
}

TEST_F(ReaderWriterTest, Pli) {
    auto packet = Buffer::Create(g_system_allocator, 1500);

    Writer writer(packet.GetViewWithCapacity());
    ASSERT_TRUE(PliWriter::Write(writer, _sender_ssrc, _media_ssrc));
    packet.SetSize(writer.GetSize());

    const auto view = ToConst(packet.GetView());
    ASSERT_TRUE(Reader::Validate(view));
    ASSERT_EQ(_sender_ssrc, PliReader::GetSenderSsrc(view));
    ASSERT_EQ(_media_ssrc, PliReader::GetMediaSsrc(view));
}

TEST_F(ReaderWriterTest, Fir) {
    auto packet = Buffer::Create(g_system_allocator, 1500);
    const auto sn = g_random.Int<uint8_t>();

    Writer writer(packet.GetViewWithCapacity());
    ASSERT_TRUE(FirWriter::Write(writer, _sender_ssrc, _media_ssrc, sn));
    packet.SetSize(writer.GetSize());

    const auto view = ToConst(packet.GetView());
    ASSERT_TRUE(Reader::Validate(view));
    ASSERT_EQ(_sender_ssrc, FirReader::GetSenderSsrc(view));
    ASSERT_EQ(_media_ssrc, FirReader::GetMediaSsrc(view));
    ASSERT_EQ(sn, FirReader::GetSn(view));
}

TEST_F(ReaderWriterTest, Nack) {
    auto packet = Buffer::Create(g_system_allocator, 1500);
    const NackSns nack_sns = {1, 2, 3, 5, 8, 13, 21, 34, 55, 89};

    Writer writer(packet.GetViewWithCapacity());
    ASSERT_TRUE(NackWriter::Write(writer, _sender_ssrc, _media_ssrc, nack_sns));
    packet.SetSize(writer.GetSize());

    const auto view = ToConst(packet.GetView());
    ASSERT_TRUE(Reader::Validate(view));
    ASSERT_EQ(_sender_ssrc, NackReader::GetSenderSsrc(view));
    ASSERT_EQ(_media_ssrc, NackReader::GetMediaSsrc(view));
    ASSERT_EQ(nack_sns, NackReader::GetSns(view));
}

TEST_F(ReaderWriterTest, Nack_Randomized) {
    for(size_t i = 0; i < 100; ++i) {
        auto packet = Buffer::Create(g_system_allocator, 1500);
        const auto nack_sns = CreateNackSns(g_random.Int<size_t>(1, 100));

        Writer writer(packet.GetViewWithCapacity());
        ASSERT_TRUE(NackWriter::Write(writer, _sender_ssrc, _media_ssrc, nack_sns));
        packet.SetSize(writer.GetSize());

        const auto view = ToConst(packet.GetView());
        ASSERT_TRUE(Reader::Validate(view));
        ASSERT_EQ(_sender_ssrc, NackReader::GetSenderSsrc(view));
        ASSERT_EQ(_media_ssrc, NackReader::GetMediaSsrc(view));
        ASSERT_EQ(nack_sns, NackReader::GetSns(view));
    }
}

TEST_F(ReaderWriterTest, WrongNack_EmptySns) {
    auto packet = Buffer::Create(g_system_allocator, 1500);
    Writer writer(packet.GetViewWithCapacity());
    ASSERT_FALSE(NackWriter::Write(writer, _sender_ssrc, _media_ssrc, {}));
}

TEST_F(ReaderWriterTest, Compound_Randomized) {
    for(size_t i = 0; i < 100; ++i) {
        auto packet = Buffer::Create(g_system_allocator, 1500);
        const auto info = CreateSrInfo();
        const auto rr_blocks = CreateRrBlocks(g_random.Int<size_t>(1, 10));
        const auto bye_ssrcs = ByeSsrcs(g_random.Int<size_t>(0, 30));
        const auto nack_sns = CreateNackSns(g_random.Int<size_t>(1, 100));
        const auto fir_sn = g_random.Int<uint8_t>();

        Writer writer(packet.GetViewWithCapacity());
        size_t reports_count = 0;
        if(g_random.Bool())  {
            ASSERT_TRUE(SrWriter::Write(writer, _sender_ssrc, info, rr_blocks));
            reports_count++;
        }
        if(g_random.Bool())  {
            ASSERT_TRUE(RrWriter::Write(writer, _sender_ssrc, rr_blocks));
            reports_count++;
        }
        if(g_random.Bool())  {
            ASSERT_TRUE(ByeWriter::Write(writer, bye_ssrcs));
            reports_count++;
        }
        if(g_random.Bool())  {
            ASSERT_TRUE(NackWriter::Write(writer, _sender_ssrc, _media_ssrc, nack_sns));
            reports_count++;
        }
        if(g_random.Bool())  {
            ASSERT_TRUE(PliWriter::Write(writer, _sender_ssrc, _media_ssrc));
            reports_count++;
        }
        if(g_random.Bool() || (reports_count == 0))  {
            ASSERT_TRUE(FirWriter::Write(writer, _sender_ssrc, _media_ssrc, fir_sn));
            reports_count++;
        }
        packet.SetSize(writer.GetSize());

        const auto view = ToConst(packet.GetView());
        ASSERT_TRUE(Reader::Validate(view));
        size_t checks_count = 0;
        Reader::ForEachReport(view, [&](Type type, const BufferViewConst& report) {
            if(type == Type::kSr) {
                EXPECT_EQ(_sender_ssrc, SrReader::GetSenderSsrc(report));
                EXPECT_EQ(info, SrReader::GetSrInfo(report));
                EXPECT_NO_FATAL_FAILURE(AssertRrBlocks(rr_blocks, SrReader::GetBlocks(report)));
                checks_count++;
            } else if(type == Type::kRr) {
                EXPECT_EQ(_sender_ssrc, RrReader::GetSenderSsrc(report));
                EXPECT_NO_FATAL_FAILURE(AssertRrBlocks(rr_blocks, RrReader::GetBlocks(report)));
                checks_count++;
            } else if(type == Type::kBye) {
                EXPECT_EQ(bye_ssrcs, ByeReader::GetSsrcs(report));
                checks_count++;
            } else if(type == Type::kRtpfb) {
                EXPECT_EQ(RtpfbType::kNack, GetRc(report.ptr[0]));
                EXPECT_EQ(_sender_ssrc, NackReader::GetSenderSsrc(report));
                EXPECT_EQ(_media_ssrc, NackReader::GetMediaSsrc(report));
                EXPECT_EQ(nack_sns, NackReader::GetSns(report));
                checks_count++;
            } else if(type == Type::kPsfb) {
                switch(GetRc(report.ptr[0])) {
                    case PsfbType::kPli:
                        EXPECT_EQ(_sender_ssrc, PliReader::GetSenderSsrc(report));
                        EXPECT_EQ(_media_ssrc, PliReader::GetMediaSsrc(report));
                        checks_count++;
                        break;
                    case PsfbType::kFir:
                        EXPECT_EQ(_sender_ssrc, FirReader::GetSenderSsrc(report));
                        EXPECT_EQ(_media_ssrc, FirReader::GetMediaSsrc(report));
                        EXPECT_EQ(fir_sn, FirReader::GetSn(report));
                        checks_count++;
                        break;
                }
            }
            return true;
        });
        ASSERT_EQ(reports_count, checks_count);
    }
}

TEST_F(ReaderWriterTest, Padding) {
    auto packet = Buffer::Create(g_system_allocator, 1500);
    const auto padding_size = 2 * sizeof(uint32_t);

    Writer writer(packet.GetViewWithCapacity());
    ASSERT_TRUE(FirWriter::Write(writer, _sender_ssrc, _media_ssrc, 1));
    writer.WriteHeader(Type::kPsfb, PsfbType::kPli, kHeaderSize + 2 * sizeof(uint32_t) + padding_size, true);
    writer.Write(_sender_ssrc);
    writer.Write(_media_ssrc);
    writer.Write(static_cast<uint32_t>(0));
    writer.Write(static_cast<uint32_t>(padding_size));
    packet.SetSize(writer.GetSize());
    const auto pli_target_size_without_padding = kHeaderSize + 2 * sizeof(uint32_t);

    const auto view = ToConst(packet.GetView());
    ASSERT_TRUE(Reader::Validate(view));
    bool pli_checked = false;
    bool fir_checked = false;
    Reader::ForEachReport(view, [&](Type type, const BufferViewConst& report) {
        EXPECT_EQ(Type::kPsfb, type);
        switch(GetRc(report.ptr[0])) {
            case PsfbType::kPli:
                EXPECT_EQ(pli_target_size_without_padding, report.size);        
                EXPECT_TRUE(HasPadding(report.ptr[0]));
                EXPECT_EQ(_sender_ssrc, PliReader::GetSenderSsrc(report));
                EXPECT_EQ(_media_ssrc, PliReader::GetMediaSsrc(report));
                pli_checked = true;
                break;
            case PsfbType::kFir:
                EXPECT_FALSE(HasPadding(report.ptr[0]));
                EXPECT_EQ(_sender_ssrc, FirReader::GetSenderSsrc(report));
                EXPECT_EQ(_media_ssrc, FirReader::GetMediaSsrc(report));
                fir_checked = true;
                break;
            default:
                EXPECT_TRUE(false);
                break;
        }
        return true;
    });
    ASSERT_TRUE(pli_checked);
    ASSERT_TRUE(fir_checked);
}

TEST_F(ReaderWriterTest, WrongPaddingSize) {
    auto packet = Buffer::Create(g_system_allocator, 1500);
    const auto padding_size = 2 * sizeof(uint32_t);

    Writer writer(packet.GetViewWithCapacity());
    writer.WriteHeader(Type::kPsfb, PsfbType::kPli, kHeaderSize + 2 * sizeof(uint32_t) + padding_size, true);
    writer.Write(_sender_ssrc);
    writer.Write(_media_ssrc);

    const auto padding_with_header_bigger_than_report_size = sizeof(_sender_ssrc) + sizeof(_media_ssrc) + padding_size + 1;
    writer.Write(static_cast<uint32_t>(0));
    writer.Write(static_cast<uint32_t>(padding_with_header_bigger_than_report_size));

    packet.SetSize(writer.GetSize());

    const auto view = ToConst(packet.GetView());
    ASSERT_FALSE(Reader::Validate(view));
}

TEST_F(ReaderWriterTest, WrongPadding_NotLastReport) {
    auto packet = Buffer::Create(g_system_allocator, 1500);
    const auto padding_size = 2 * sizeof(uint32_t);

    Writer writer(packet.GetViewWithCapacity());
    writer.WriteHeader(Type::kPsfb, PsfbType::kPli, kHeaderSize + 2 * sizeof(uint32_t) + padding_size, true);
    writer.Write(_sender_ssrc);
    writer.Write(_media_ssrc);
    writer.Write(static_cast<uint32_t>(0));
    writer.Write(static_cast<uint32_t>(padding_size));
    ASSERT_TRUE(FirWriter::Write(writer, _sender_ssrc, _media_ssrc, 1));
    packet.SetSize(writer.GetSize());

    const auto view = ToConst(packet.GetView());
    ASSERT_FALSE(Reader::Validate(view));
}

TEST_F(ReaderWriterTest, WrongBufferSizeForWriting) {
    auto packet = Buffer::Create(g_system_allocator, kHeaderSize);
    Writer writer(packet.GetViewWithCapacity());
    ASSERT_FALSE(SrWriter::Write(writer, _sender_ssrc, CreateSrInfo(), {}));
    ASSERT_EQ(0, writer.GetSize());
}

}
