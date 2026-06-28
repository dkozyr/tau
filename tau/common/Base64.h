#pragma once

#include <etl/string.h>
#include <etl/string_view.h>

namespace tau {

etl::istring& Base64Decode(etl::string_view input, etl::istring& decoded);
etl::istring& Base64Encode(etl::string_view input, etl::istring& encoded);

inline etl::istring& Base64Encode(void* data, size_t size, etl::istring& encoded) {
    return Base64Encode(etl::string_view{reinterpret_cast<const char*>(data), size}, encoded);
}

}
