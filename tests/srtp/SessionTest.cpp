#include "tau/srtp/Session.h"
#include "tau/srtp/Common.h"
#include "tau/crypto/Random.h"
#include "tau/rtp/Writer.h"
#include "tau/rtcp/SrWriter.h"
#include "tests/lib/Common.h"

namespace tau::srtp {

struct SessionTestParams {
    srtp_profile_t profile;
};

const std::vector<SessionTestParams> kSessionTestParamsVec = {
    {.profile = srtp_profile_t::srtp_profile_aes128_cm_sha1_80},
    {.profile = srtp_profile_t::srtp_profile_aes128_cm_sha1_32},
    {.profile = srtp_profile_t::srtp_profile_null_sha1_80},
    {.profile = srtp_profile_t::srtp_profile_aead_aes_128_gcm},
    {.profile = srtp_profile_t::srtp_profile_aead_aes_256_gcm},
};

class SessionTest : public ::testing::TestWithParam<SessionTestParams> {
public:
    SessionTest()
        : _key(SRTP_MAX_KEY_LEN) // max key size to cover all test cases
    { 
        crypto::RandomBytes(_key.data(), _key.size());

        _encryptor.emplace(Session::Options{
            .type = Session::Type::kEncryptor,
            .profile = GetParam().profile,
            .key = _key,
            .log_ctx = "[test] "
        });
        _decryptor.emplace(Session::Options{
            .type = Session::Type::kDecryptor,
            .profile = GetParam().profile,
            .key = _key,
            .log_ctx = "[test] "
        });

        _encryptor->SetCallback([this](Buffer&& encrypted, bool is_rtp) {
            ASSERT_EQ(is_rtp, !rtcp::IsRtcp(ToConst(encrypted.GetView())));
            _encrypted.emplace_back(std::move(encrypted));
        });
        _decryptor->SetCallback([this](Buffer&& decrypted, bool is_rtp) {
            ASSERT_EQ(is_rtp, !rtcp::IsRtcp(ToConst(decrypted.GetView())));
            _decrypted.emplace_back(std::move(decrypted));
        });
    }

    Buffer CreateRtpPacket() {
        auto size = g_random.Int<size_t>(12, 1200);
        auto packet = Buffer::Create(g_udp_allocator);
        packet.SetSize(size);

        _rtp_options.marker = g_random.Bool();
        auto result = rtp::Writer::Write(packet.GetViewWithCapacity(), _rtp_options);
        EXPECT_LT(0, result.size);
        _rtp_options.sn++;

        for(size_t i = result.size; i < size; ++i) {
            packet.GetView().ptr[i] = i;
        }
        return packet;
    }

    static Buffer CreateRtcpPacket() {
        auto packet = Buffer::Create(g_system_allocator);
        const auto info = CreateSrInfo();
        const auto rr_blocks = CreateRrBlocks(g_random.Int(1, 32));

        rtcp::Writer writer(packet.GetViewWithCapacity());
        EXPECT_TRUE(rtcp::SrWriter::Write(writer, g_random.Int<uint32_t>(), info, rr_blocks));
        packet.SetSize(writer.GetSize());
        return packet;
    }

    //TODO: move to utils files?
    static rtcp::SrInfo CreateSrInfo() {
        return rtcp::SrInfo{
            .ntp          = g_random.Int<uint64_t>(),
            .ts           = g_random.Int<uint32_t>(),
            .packet_count = g_random.Int<uint32_t>(),
            .octet_count  = g_random.Int<uint32_t>()
        };
    }

    static rtcp::RrBlocks CreateRrBlocks(uint8_t blocks_count) {
        rtcp::RrBlocks blocks;
        for(uint8_t i = 0; i < blocks_count; ++i) {
            blocks.push_back(CreateRrBlock());
        }
        return blocks;
    }

    static rtcp::RrBlock CreateRrBlock() {
        return rtcp::RrBlock{
            .ssrc             = g_random.Int<uint32_t>(),
            .packet_lost_word = g_random.Int<uint32_t>(),
            .ext_highest_sn   = g_random.Int<uint32_t>(),
            .jitter           = g_random.Int<uint32_t>(),
            .lsr              = g_random.Int<uint32_t>(),
            .dlsr             = g_random.Int<uint32_t>()
        };
    }

    static void AssertPackets(const Buffer& packet, const Buffer& encrypted, const Buffer& decrypted) {
        auto view = packet.GetView();
        auto view_decrypted = decrypted.GetView();
        ASSERT_EQ(view.size, view_decrypted.size);
        ASSERT_LT(view.size, encrypted.GetSize());
        for(size_t i = 0; i < view.size; ++i) {
            ASSERT_EQ(view.ptr[i], view_decrypted.ptr[i]);
        }
    }

protected:
    std::vector<uint8_t> _key;
    std::optional<Session> _encryptor;
    std::optional<Session> _decryptor;
    rtp::Writer::Options _rtp_options{
        .pt = g_random.Int<uint8_t>(96, 127),
        .ssrc = g_random.Int<uint32_t>(),
        .ts = g_random.Int<uint32_t>(),
        .sn = g_random.Int<uint16_t>(),
        .marker = false,
        .extension_length_in_words = 0
    };
    std::vector<Buffer> _encrypted;
    std::vector<Buffer> _decrypted;
};

INSTANTIATE_TEST_SUITE_P(Parametrized, SessionTest, ::testing::ValuesIn(kSessionTestParamsVec.begin(), kSessionTestParamsVec.end()));

TEST_P(SessionTest, Rtp) {
    auto packet = CreateRtpPacket();
    ASSERT_TRUE(_encryptor->Encrypt(packet.MakeCopy()));
    ASSERT_EQ(1, _encrypted.size());
    ASSERT_TRUE(_decryptor->Decrypt(_encrypted[0].MakeCopy()));
    ASSERT_EQ(1, _decrypted.size());
    ASSERT_NO_FATAL_FAILURE(AssertPackets(packet, _encrypted[0], _decrypted[0]));
}

TEST_P(SessionTest, RtpReplayWindow) {
    auto packet1 = CreateRtpPacket();
    auto packet2 = CreateRtpPacket();
    ASSERT_TRUE(_encryptor->Encrypt(packet1.MakeCopy()));
    ASSERT_TRUE(_encryptor->Encrypt(packet2.MakeCopy()));

    for(size_t i = 0; i < Session::kRtxWindowSize - 1; ++i) {
        ASSERT_TRUE(_encryptor->Encrypt(CreateRtpPacket()));
        ASSERT_EQ(2 + 1 + i, _encrypted.size());
    }
    ASSERT_FALSE(_encryptor->Encrypt(packet1.MakeCopy())); // too old
    ASSERT_TRUE(_encryptor->Encrypt(packet2.MakeCopy()));  // oldest in the replay window
}

TEST_P(SessionTest, Rtcp) {
    auto packet = CreateRtcpPacket();
    ASSERT_TRUE(_encryptor->Encrypt(packet.MakeCopy(), false));
    ASSERT_EQ(1, _encrypted.size());
    ASSERT_TRUE(_decryptor->Decrypt(_encrypted[0].MakeCopy(), false));
    ASSERT_EQ(1, _decrypted.size());
    ASSERT_NO_FATAL_FAILURE(AssertPackets(packet, _encrypted[0], _decrypted[0]));
}

TEST(SessionTest, WrongProfile) {
    ASSERT_ANY_THROW(Session(Session::Options{
        .type = Session::Type::kEncryptor,
        .profile = srtp_profile_t::srtp_profile_null_sha1_32,
        .key = {},
        .log_ctx = "[test] "
    }));
}

}
