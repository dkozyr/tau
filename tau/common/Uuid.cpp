#include "tau/common/String.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace tau {

std::string GenerateUuid() {
    return boost::uuids::to_string(boost::uuids::random_generator()());
}

bool IsUuidTrivialCheck(std::string_view uuid) {
    if(uuid.size() == 36) {
        auto v = Split(uuid, "-");
        if(v.size() == 5) {
            return (v[0].size() == 8) && (v[1].size() == 4) && (v[2].size() == 4) && (v[3].size() == 4);
        }
    }
    return false;
}

}
