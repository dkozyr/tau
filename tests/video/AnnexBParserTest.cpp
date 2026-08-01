#include "tau/video/AnnexBParser.h"
#include "tests/lib/Common.h"

namespace tau::video {

TEST(AnnexBParserTest, Basic) {
    etl::array<uint8_t, 20> data = {0, 0, 0, 1, 2, 3, 4, 0, 0, 1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 1};

    etl::vector<BufferView, 16> nal_units;
    {
        BufferView au{.ptr = data.data(), .size = data.size()};
        ASSERT_EQ(20, ParseAnnexB(au, nal_units));

        ASSERT_EQ(3, nal_units.size());
        ASSERT_EQ(7, nal_units[0].size);
        ASSERT_EQ(9, nal_units[1].size);
        ASSERT_EQ(4, nal_units[2].size);
    }
    {
        BufferView au{.ptr = data.data() + 1, .size = data.size() - 1};
        ASSERT_EQ(19, ParseAnnexB(au, nal_units));

        ASSERT_EQ(3, nal_units.size());
        ASSERT_EQ(6, nal_units[0].size);
        ASSERT_EQ(9, nal_units[1].size);
        ASSERT_EQ(4, nal_units[2].size);
    }
    {
        BufferView au{.ptr = data.data() + 1, .size = data.size() - 2};
        ASSERT_EQ(18, ParseAnnexB(au, nal_units));

        ASSERT_EQ(2, nal_units.size());
        ASSERT_EQ(6, nal_units[0].size);
        ASSERT_EQ(12, nal_units[1].size);
    }
    {
        BufferView au{.ptr = data.data() + 2, .size = data.size()};
        ASSERT_EQ(0, ParseAnnexB(au, nal_units));

        ASSERT_EQ(0, nal_units.size());
    }
}

TEST(AnnexBParserTest, ProcessByChunks) {
    etl::array<uint8_t, 20> data = {0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 1};
    {
        etl::vector<BufferView, 2> nal_units;

        BufferView au{.ptr = data.data(), .size = data.size()};
        ASSERT_EQ(7, ParseAnnexB(au, nal_units));
        ASSERT_EQ(2, nal_units.size());
        ASSERT_EQ(4, nal_units[0].size);
        ASSERT_EQ(3, nal_units[1].size);

        au.ForwardPtrUnsafe(7);
        ASSERT_EQ(13, ParseAnnexB(au, nal_units));
        ASSERT_EQ(2, nal_units.size());
        ASSERT_EQ(9, nal_units[0].size);
        ASSERT_EQ(4, nal_units[1].size);
    }
    {
        etl::vector<BufferView, 1> nal_units;

        BufferView au{.ptr = data.data(), .size = data.size()};
        ASSERT_EQ(4, ParseAnnexB(au, nal_units));
        ASSERT_EQ(1, nal_units.size());
        ASSERT_EQ(4, nal_units[0].size);

        au.ForwardPtrUnsafe(nal_units[0].size);
        ASSERT_EQ(3, ParseAnnexB(au, nal_units));
        ASSERT_EQ(1, nal_units.size());
        ASSERT_EQ(3, nal_units[0].size);

        au.ForwardPtrUnsafe(nal_units[0].size);
        ASSERT_EQ(9, ParseAnnexB(au, nal_units));
        ASSERT_EQ(1, nal_units.size());
        ASSERT_EQ(9, nal_units[0].size);

        au.ForwardPtrUnsafe(nal_units[0].size);
        ASSERT_EQ(4, ParseAnnexB(au, nal_units));
        ASSERT_EQ(1, nal_units.size());
        ASSERT_EQ(4, nal_units[0].size);
    }
}

TEST(AnnexBParserTest, Randomized) {
    std::vector<uint8_t> data;
    for(size_t iter = 0; iter < 100; ++iter) {
        std::vector<size_t> nal_unit_sizes(g_random.Int<size_t>(1, 15));
        for(size_t i = 0; i < nal_unit_sizes.size(); ++i) {
            const auto payload_size = g_random.Int<size_t>(1, 16'000);
            if(g_random.Bool()) {
                nal_unit_sizes[i] = payload_size + kAnnexB.size();
                data.insert(data.end(), kAnnexB.begin(), kAnnexB.end());
            } else {
                nal_unit_sizes[i] = payload_size + kAnnexBShort.size();
                data.insert(data.end(), kAnnexBShort.begin(), kAnnexBShort.end());
            }
            for(size_t j = 0; j < payload_size; ++j) {
                data.push_back(g_random.Int<uint8_t>(1, 0xFF));
            }
        }

        etl::vector<BufferView, 16> nal_units;
        BufferView au{.ptr = data.data(), .size = data.size()};
        ParseAnnexB(au, nal_units);

        ASSERT_EQ(nal_unit_sizes.size(), nal_units.size());
        for(size_t i = 0; i < nal_unit_sizes.size(); ++i) {
            ASSERT_EQ(nal_unit_sizes[i], nal_units[i].size);
        }

        data.clear();
    }
}

TEST(AnnexBParserTest, GetStartCodeLength) {
    etl::array<uint8_t, 20> data = {0, 0, 0, 1, 2, 3, 4, 0, 0, 1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 1};
    BufferView view{.ptr = data.data(), .size = data.size()};

    ASSERT_EQ(4, GetStartCodeLength(view, 0));
    ASSERT_EQ(3, GetStartCodeLength(view, 1));
    ASSERT_EQ(0, GetStartCodeLength(view, 2));
    ASSERT_EQ(0, GetStartCodeLength(view, 3));
    ASSERT_EQ(0, GetStartCodeLength(view, 4));
    ASSERT_EQ(0, GetStartCodeLength(view, 5));
    ASSERT_EQ(0, GetStartCodeLength(view, 6));
    ASSERT_EQ(3, GetStartCodeLength(view, 7));
    ASSERT_EQ(0, GetStartCodeLength(view, 8));
    ASSERT_EQ(0, GetStartCodeLength(view, 9));
    ASSERT_EQ(0, GetStartCodeLength(view, 10));
    ASSERT_EQ(0, GetStartCodeLength(view, 11));
    ASSERT_EQ(0, GetStartCodeLength(view, 12));
    ASSERT_EQ(0, GetStartCodeLength(view, 13));
    ASSERT_EQ(0, GetStartCodeLength(view, 14));
    ASSERT_EQ(0, GetStartCodeLength(view, 15));
    ASSERT_EQ(4, GetStartCodeLength(view, 16));
    ASSERT_EQ(3, GetStartCodeLength(view, 17));
    ASSERT_EQ(0, GetStartCodeLength(view, 18));
    ASSERT_EQ(0, GetStartCodeLength(view, 19));
    ASSERT_EQ(0, GetStartCodeLength(view, 1000));
}

}
