#include "tau/common/Uuid.h"
#include "tau/common/String.h"
#include "tau/common/Random.h"

namespace tau {

const etl::string_view kHex = "0123456789abcdef-";

Uuid GenerateUuid() {
    Uuid uuid(kUuidSize, 0);
    Random random;
    for(size_t i = 0; i < kUuidSize; ++i) {
        uint8_t v = 16;
        switch(i) {
            case 8:
            case 13:
            case 18:
            case 23:
                break;
            case 14:
                v = 4;
                break;
            case 19:
                v = random.Int<uint8_t>(8, 11);
                break;
            default:
                v = random.Int<uint8_t>(0, 15);
                break;
        }
        uuid[i] = kHex[v];
    }
    return uuid;
}

bool IsUuidTrivialCheck(etl::string_view uuid) {
    size_t minus = 0;
    size_t alpha_digit = 0;
    for(auto c : uuid) {
        if(c == '-') {
            minus++;
        } else if(IsAlphaDigit(c)) {
            alpha_digit++;
        } else {
            return false;
        }
    }
    return (minus == 4) && (alpha_digit == 32) &&
           (uuid[8] == '-') && (uuid[13] == '-') && (uuid[18] == '-') && (uuid[23] == '-');
}

}
