#include <tau/crypto/Random.h>
#include <tau/common/Base64.h>
#include <tau/common/Math.h>

namespace tau::crypto {

etl::istring& RandomBase64(etl::istring& output, size_t size) {
    const auto data_size = DivCeil(size * 6, 8);
    auto data = reinterpret_cast<uint8_t*>(malloc(data_size));
    RandomBytes(data, data_size);
    Base64Encode(data, data_size, output);
    output.resize(size);
    free(data);
    return output;
}

}
