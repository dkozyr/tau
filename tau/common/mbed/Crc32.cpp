#include <tau/common/Crc32.h>
#include "esp_crc.h"

namespace tau {

uint32_t Crc32(const uint8_t* data, size_t size) {
    return esp_crc32_le(0, data, size);
}

}
