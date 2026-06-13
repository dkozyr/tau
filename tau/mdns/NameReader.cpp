#include "tau/mdns/NameReader.h"

namespace tau::mdns {

Name ParseName(const uint8_t*& ptr, const uint8_t* end) {
    Name name;
    while(ptr < end) {
        auto size = ptr[0];
        if(size == 0) {
            ptr++;
            break;
        }
        if((ptr[0] & 0xC0) == 0xC0) {
            while(ptr < end) {
                if(ptr + 1 >= end) {
                    return {};
                }
                if((ptr[0] & 0xC0) != 0xC0) {
                    break;
                }
                name += ".$";
                ptr += sizeof(uint16_t);
            }
            break;
        }
        if(ptr + 1 + size > end) {
            return {};
        }
        if(!name.empty()) {
            name += '.';
        }
        name.append(reinterpret_cast<const char*>(ptr + 1), size);
        ptr += 1 + size;
    }
    return (ptr <= end) ? name : Name{};
}

}
