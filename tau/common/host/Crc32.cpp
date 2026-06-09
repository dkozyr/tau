#include <tau/common/Crc32.h>
#include <boost/crc.hpp>

namespace tau {

uint32_t Crc32(const uint8_t* data, size_t size) {
    boost::crc_32_type crc32;
    crc32.process_bytes(data, size);
    return crc32.checksum();
}

}
