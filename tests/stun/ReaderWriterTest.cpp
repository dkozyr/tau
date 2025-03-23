#include "tau/stun/Reader.h"
#include "tau/stun/Writer.h"
#include "tau/stun/AttributeType.h"
#include "tau/stun/attribute/XorMappedAddress.h"
#include "tau/stun/attribute/Priority.h"
#include "tau/stun/attribute/IceControlled.h"
#include "tau/stun/attribute/IceControlling.h"
#include "tau/stun/attribute/UserName.h"
#include "tau/stun/attribute/MessageIntegrity.h"
#include "tau/stun/attribute/Fingerprint.h"
#include "tests/lib/Common.h"

namespace tau::stun {

using namespace tau::stun::attribute;

class ReaderWriterTest : public ::testing::Test {
public:
    ReaderWriterTest()
        : _packet(Buffer::Create(g_system_allocator, kUdpMtuSize))
        , _transaction_id_hash(GenerateTransactionId(_transaction_id.data()))
    {}

protected:
    Buffer _packet;
    std::array<uint8_t, kTransactionIdSize> _transaction_id;
    uint32_t _transaction_id_hash;
};

TEST_F(ReaderWriterTest, Basic) {
    Writer writer(_packet.GetViewWithCapacity());
    writer.WriteHeader(BindingType::kRequest, BufferViewConst{_transaction_id.data(), _transaction_id.size()});
    size_t target_size = kMessageHeaderSize;
    ASSERT_EQ(target_size, writer.GetSize());

    XorMappedAddressWriter::Write(writer, 0x12345678, 12345);
    target_size += kAttributeHeaderSize + IPv4PayloadSize;
    ASSERT_EQ(target_size, writer.GetSize());

    PriorityWriter::Write(writer, 0x23456789);
    target_size += kAttributeHeaderSize + sizeof(uint32_t);
    ASSERT_EQ(target_size, writer.GetSize());

    IceControlledWriter::Write(writer, 0x0123456789ABCDEF);
    target_size += kAttributeHeaderSize + sizeof(uint64_t);
    ASSERT_EQ(target_size, writer.GetSize());

    IceControllingWriter::Write(writer, 0x123456789ABCDEF0);
    target_size += kAttributeHeaderSize + sizeof(uint64_t);
    ASSERT_EQ(target_size, writer.GetSize());

    UserNameWriter::Write(writer, "hello", "world");
    target_size += kAttributeHeaderSize + Align(5 + 5 + 1, sizeof(uint32_t));
    ASSERT_EQ(target_size, writer.GetSize());

    MessageIntegrityWriter::Write(writer, "pas$word");
    target_size += kAttributeHeaderSize + MessageIntegrityPayloadSize;
    ASSERT_EQ(target_size, writer.GetSize());

    FingerprintWriter::Write(writer);
    target_size += kAttributeHeaderSize + sizeof(uint32_t);
    ASSERT_EQ(target_size, writer.GetSize());

    _packet.SetSize(writer.GetSize());

    auto view = ToConst(_packet.GetView());
    ASSERT_TRUE(Reader::Validate(view));
    ASSERT_EQ(_transaction_id_hash, HeaderReader::GetTransactionIdHash(view));
    std::vector<AttributeType> attributes;
    auto ok = Reader::ForEachAttribute(view, [&](AttributeType type, const BufferViewConst& attr) {
        attributes.push_back(type);
        switch(type) {
            case AttributeType::kXorMappedAddress:
                EXPECT_EQ(IpFamily::kIpv4,    XorMappedAddressReader::GetFamily(attr));
                EXPECT_EQ(12345,              XorMappedAddressReader::GetPort(attr));
                EXPECT_EQ(0x12345678,         XorMappedAddressReader::GetAddressV4(attr));
                break;
            case AttributeType::kPriority:
                EXPECT_EQ(0x23456789,         PriorityReader::GetPriority(attr));
                break;
            case AttributeType::kIceControlled:
                EXPECT_EQ(0x0123456789ABCDEF, IceControlledReader::GetTiebreaker(attr));
                break;
            case AttributeType::kIceControlling:
                EXPECT_EQ(0x123456789ABCDEF0, IceControllingReader::GetTiebreaker(attr));
                break;
            case AttributeType::kUserName:
                EXPECT_EQ("world:hello",      UserNameReader::GetUserName(attr));
                break;
            case AttributeType::kMessageIntegrity:
                EXPECT_EQ(true,               MessageIntegrityReader::Validate(attr, view, "pas$word"));
                break;
            case AttributeType::kFingerprint:
                EXPECT_EQ(true,               FingerprintReader::Validate(attr, view));
                break;
            default:
                EXPECT_TRUE(false);
                return false;
        }
        return true;
    });
    ASSERT_TRUE(ok);
    std::vector<AttributeType> target_attributes = {AttributeType::kXorMappedAddress, AttributeType::kPriority,
        AttributeType::kIceControlled, AttributeType::kIceControlling, AttributeType::kUserName,
        AttributeType::kMessageIntegrity, AttributeType::kFingerprint};
    ASSERT_EQ(target_attributes, attributes);
}

}
