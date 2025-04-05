#pragma once

#include <cstdint>

namespace tau::stun {

enum Method : uint16_t {
    kBinding          = 0x0001,
    kAllocate         = 0x0003,
    kRefresh          = 0x0004,
    kSend             = 0x0006,
    kData             = 0x0007,
    kCreatePermission = 0x0008,
    kChannelBind      = 0x0009,
};

enum Type : uint16_t {
    kRequest       = 0x0000,
    kIndication    = 0x0010,
    kResponse      = 0x0100,
    kErrorResponse = 0x0110
};

inline constexpr uint16_t MessageType(Method method, Type type) {
    return static_cast<uint16_t>(method) | static_cast<uint16_t>(type);
}

inline constexpr uint16_t kBindingRequest           = MessageType(Method::kBinding,  Type::kRequest);
inline constexpr uint16_t kBindingResponse          = MessageType(Method::kBinding,  Type::kResponse);
inline constexpr uint16_t kBindingIndication        = MessageType(Method::kBinding,  Type::kIndication);
inline constexpr uint16_t kBindingErrorResponse     = MessageType(Method::kBinding,  Type::kErrorResponse);
inline constexpr uint16_t kAllocateRequest          = MessageType(Method::kAllocate, Type::kRequest);
inline constexpr uint16_t kAllocateResponse         = MessageType(Method::kAllocate, Type::kResponse);
inline constexpr uint16_t kAllocateErrorResponse    = MessageType(Method::kAllocate, Type::kErrorResponse);
inline constexpr uint16_t kRefreshRequest           = MessageType(Method::kRefresh,  Type::kRequest);
inline constexpr uint16_t kRefreshResponse          = MessageType(Method::kRefresh,  Type::kResponse);
inline constexpr uint16_t kCreatePermissionRequest  = MessageType(Method::kCreatePermission, Type::kRequest);
inline constexpr uint16_t kCreatePermissionResponse = MessageType(Method::kCreatePermission, Type::kResponse);
inline constexpr uint16_t kSendIndication           = MessageType(Method::kSend,     Type::kIndication);
inline constexpr uint16_t kDataIndication           = MessageType(Method::kData,     Type::kIndication);

}
