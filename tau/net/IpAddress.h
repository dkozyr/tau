#pragma once

#include <etl/array.h>
#include <etl/string.h>
#include <etl/string_view.h>
#include <etl/string_stream.h>
#include <cstdint>

namespace tau::net {

using IpAddressStrV4 = etl::string<15>; //xxx.xxx.xxx.xxx

//TODO: support ipv6
struct IpAddress {
    etl::array<uint8_t, 4> bytes = {0, 0, 0, 0}; //TODO: uint32_t?

    IpAddress() = default;

    IpAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
        : bytes{a, b, c, d}
    {}

    IpAddress(uint32_t value, bool network_order = false) {
        if(network_order) {
            bytes[0] =  value        & 0xFF;
            bytes[1] = (value >> 8)  & 0xFF;
            bytes[2] = (value >> 16) & 0xFF;
            bytes[3] = (value >> 24) & 0xFF;
        } else {
            bytes[0] = (value >> 24) & 0xFF;
            bytes[1] = (value >> 16) & 0xFF;
            bytes[2] = (value >> 8)  & 0xFF;
            bytes[3] =  value        & 0xFF;
        }
    }

    uint32_t GetUint32(bool network_order = false) const {
        if(network_order) {
            return (static_cast<uint32_t>(bytes[0]) << 0) |
                   (static_cast<uint32_t>(bytes[1]) << 8) |
                   (static_cast<uint32_t>(bytes[2]) << 16) |
                   (static_cast<uint32_t>(bytes[3]) << 24);
        } else {
            return (static_cast<uint32_t>(bytes[0]) << 24) |
                   (static_cast<uint32_t>(bytes[1]) << 16) |
                   (static_cast<uint32_t>(bytes[2]) << 8) |
                   (static_cast<uint32_t>(bytes[3]) << 0);
        }
    }

    bool IsLoopback() const {
        return (bytes[0] == 127) && (bytes[1] == 0) && (bytes[2] == 0) && (bytes[3] == 1);
    }

    bool operator==(const IpAddress& other) const {
        return (bytes == other.bytes);
    } 

    bool operator!=(const IpAddress& other) const {
        return (bytes != other.bytes);
    } 
};

IpAddress MakeIpAddressV4(const etl::string_view& address_str);

IpAddressStrV4 ToString(const IpAddress& address);
etl::string_stream& ToString(etl::string_stream& ss, const IpAddress& address);
etl::string_stream& operator<<(etl::string_stream& ss, const IpAddress& address);

}

namespace tau {

using IpAddress = net::IpAddress;
using IpAddressStrV4 = net::IpAddressStrV4;

}

namespace etl {

template <>
struct hash<tau::net::IpAddress> {
    size_t operator()(const tau::net::IpAddress& ip) const noexcept {
        return static_cast<size_t>(ip.GetUint32());
    }
};

}
