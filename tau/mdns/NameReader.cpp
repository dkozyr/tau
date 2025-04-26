#include "tau/mdns/NameReader.h"

namespace tau::mdns {

std::string ParseName(const uint8_t*& ptr, const uint8_t* end) {
    std::string name;
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
        name.append(ptr + 1, ptr + 1 + size);
        ptr += 1 + size;
    }
    return (ptr <= end) ? name : std::string{};
}

}
