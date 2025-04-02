#include "tau/stun/Reader.h"
#include "tau/stun/Writer.h"
#include "tau/stun/AttributeType.h"
#include "tau/stun/attribute/XorMappedAddress.h"
#include "tau/stun/attribute/DataUint32.h"
#include "tau/stun/attribute/IceRole.h"
#include "tau/stun/attribute/UseCandidate.h"
#include "tau/stun/attribute/ByteString.h"
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
    writer.WriteHeader(BindingType::kRequest);
    auto transcation_id_ptr = _packet.GetView().ptr + 2 * sizeof(uint32_t);
    std::memcpy(transcation_id_ptr, _transaction_id.data(), _transaction_id.size());
    size_t target_size = kMessageHeaderSize;
    ASSERT_EQ(target_size, writer.GetSize());

    XorMappedAddressWriter::Write(writer, AttributeType::kXorMappedAddress, 0x12345678, 12345);
    target_size += kAttributeHeaderSize + IPv4PayloadSize;
    ASSERT_EQ(target_size, writer.GetSize());

    DataUint32Writer::Write(writer, AttributeType::kPriority, 0x23456789);
    target_size += kAttributeHeaderSize + sizeof(uint32_t);
    ASSERT_EQ(target_size, writer.GetSize());

    UseCandidateWriter::Write(writer);
    target_size += kAttributeHeaderSize;
    ASSERT_EQ(target_size, writer.GetSize());

    IceRoleWriter::Write(writer, false, 0x0123456789ABCDEF);
    target_size += kAttributeHeaderSize + sizeof(uint64_t);
    ASSERT_EQ(target_size, writer.GetSize());

    IceRoleWriter::Write(writer, true, 0x123456789ABCDEF0);
    target_size += kAttributeHeaderSize + sizeof(uint64_t);
    ASSERT_EQ(target_size, writer.GetSize());

    ByteStringWriter::Write(writer, AttributeType::kUserName, "world:hello");
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
                EXPECT_EQ(0x23456789,         DataUint32Reader::GetValue(attr));
                break;
            case AttributeType::kUseCandidate:
                break;
            case AttributeType::kIceControlled:
                EXPECT_EQ(0x0123456789ABCDEF, IceRoleReader::GetTiebreaker(attr));
                break;
            case AttributeType::kIceControlling:
                EXPECT_EQ(0x123456789ABCDEF0, IceRoleReader::GetTiebreaker(attr));
                break;
            case AttributeType::kUserName:
                EXPECT_EQ("world:hello",      ByteStringReader::GetValue(attr));
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
        AttributeType::kUseCandidate, AttributeType::kIceControlled, AttributeType::kIceControlling,
        AttributeType::kUserName, AttributeType::kMessageIntegrity, AttributeType::kFingerprint};
    ASSERT_EQ(target_attributes, attributes);
}

TEST_F(ReaderWriterTest, Wireshark_Request) {
    std::vector<uint8_t> incoming_stun_request = {
        0x00, 0x01, 0x00, 0x50,                                                                         // header
          0x21, 0x12, 0xA4, 0x42,                                                                       // magic number
          0x71, 0x78, 0x6C, 0x58, 0x76, 0x62, 0x50, 0x4E, 0x6D, 0x59, 0x35, 0x52,                       // transcation id
        0x00, 0x06, 0x00, 0x09, 0x43, 0x30, 0x4F, 0x66, 0x3A, 0x61, 0x49, 0x5A, 0x43, 0x00, 0x00, 0x00, // user name
        0xC0, 0x57, 0x00, 0x04, 0x00, 0x00, 0x03, 0xE7,                                                 // network cost
        0x80, 0x2A, 0x00, 0x08, 0xE2, 0x13, 0xAC, 0xB5, 0x0F, 0xA9, 0xC4, 0xCA,                         // ice controlling
        0x00, 0x25, 0x00, 0x00,                                                                         // use candidate
        0x00, 0x24, 0x00, 0x04, 0x6E, 0x00, 0x1E, 0xFF,                                                 // priority
        0x00, 0x08, 0x00, 0x14,                                                                         // message integrity
          0x00, 0x6B, 0x59, 0x93, 0x53, 0x84, 0xEB, 0x53, 0x0F, 0x9F, 0x3E, 0xBC, 0x1C, 0xD8, 0x1A, 0x7E, 0xD7, 0x28, 0x48, 0x69,
        0x80, 0x28, 0x00, 0x04, 0x32, 0x19, 0x78, 0x64                                                  // fingerprint
    };

    BufferViewConst view{.ptr = incoming_stun_request.data(), .size = incoming_stun_request.size()};
    ASSERT_TRUE(Reader::Validate(view));

    ASSERT_EQ(BindingType::kRequest, HeaderReader::GetType(view));
    ASSERT_EQ(80, HeaderReader::GetLength(view));
    ASSERT_EQ(0x6A430944, HeaderReader::GetTransactionIdHash(view));

    std::vector<AttributeType> attributes;
    auto ok = Reader::ForEachAttribute(view, [&](AttributeType type, const BufferViewConst& attr) {
        attributes.push_back(type);
        switch(type) {
            case AttributeType::kPriority:
                EXPECT_EQ(0x6E001EFF,         DataUint32Reader::GetValue(attr));
                break;
            case AttributeType::kIceControlling:
                EXPECT_EQ(0xE213ACB50FA9C4CA, IceRoleReader::GetTiebreaker(attr));
                break;
            case AttributeType::kUseCandidate:
                break;
            case AttributeType::kUserName:
                EXPECT_EQ("C0Of:aIZC",        ByteStringReader::GetValue(attr));
                break;
            case AttributeType::kMessageIntegrity:
                EXPECT_EQ(true,               MessageIntegrityReader::Validate(attr, view, "ZN4ykLX1bRr+BzKwm2/ZAdCV"));
                break;
            case AttributeType::kFingerprint:
                EXPECT_EQ(true,               FingerprintReader::Validate(attr, view));
                break;
            case AttributeType::kNetworkCost:
                break;
            default:
                EXPECT_TRUE(false);
                return false;
        }
        return true;
    });
    ASSERT_TRUE(ok);
    std::vector<AttributeType> target_attributes = {AttributeType::kUserName, AttributeType::kNetworkCost,
        AttributeType::kIceControlling, AttributeType::kUseCandidate, AttributeType::kPriority,
        AttributeType::kMessageIntegrity, AttributeType::kFingerprint};
    ASSERT_EQ(7, attributes.size());
    ASSERT_EQ(target_attributes, attributes);
}

}
