#include "tau/stun/Header.h"
#include "tau/stun/Writer.h"
#include "tests/lib/Common.h"

namespace tau::stun {

class HeaderReaderWriterTest : public ::testing::Test {
public:
    HeaderReaderWriterTest()
        : _transaction_id_hash(GenerateTransactionId(_transaction_id.data()))
    {}

protected:
    std::array<uint8_t, kTransactionIdSize> _transaction_id;
    uint32_t _transaction_id_hash;
};

TEST_F(HeaderReaderWriterTest, Basic) {
    auto packet = Buffer::Create(g_system_allocator, kUdpMtuSize);

    Writer writer(packet.GetViewWithCapacity(), kBindingRequest);
    auto transcation_id_ptr = packet.GetView().ptr + 2 * sizeof(uint32_t);
    std::memcpy(transcation_id_ptr, _transaction_id.data(), _transaction_id.size());
    packet.SetSize(writer.GetSize());
    ASSERT_EQ(kMessageHeaderSize, packet.GetSize());

    auto view = ToConst(packet.GetView());
    ASSERT_TRUE(HeaderReader::Validate(view));
    ASSERT_EQ(kBindingRequest, HeaderReader::GetType(view));
    ASSERT_EQ(0, HeaderReader::GetLength(view));
    ASSERT_EQ(_transaction_id_hash, HeaderReader::GetTransactionIdHash(view));

    const auto transaction_id_pos = 2 * sizeof(uint32_t) + g_random.Int(0, 11);
    packet.GetView().ptr[transaction_id_pos] ^= 0xFF;
    ASSERT_TRUE(HeaderReader::Validate(view));
    ASSERT_NE(_transaction_id_hash, HeaderReader::GetTransactionIdHash(view));
}

}
