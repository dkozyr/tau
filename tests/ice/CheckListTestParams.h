#pragma once

#include "NatEmulator.h"
#include <etl/string_stream.h>

namespace tau::ice {

struct CheckListTestParams {
    NatEmulator::Type peer1_nat_type;
    size_t peer1_sockets_count;

    NatEmulator::Type peer2_nat_type;
    size_t peer2_sockets_count;

    bool success;
};

inline etl::string_stream& operator<<(etl::string_stream& ss, const CheckListTestParams& x) {
    return ss
        << "peer1: {nat: " << x.peer1_nat_type << ", sockets: " << x.peer1_sockets_count << "}, "
        << "peer2: {nat: " << x.peer2_nat_type << ", sockets: " << x.peer2_sockets_count << "}, "
        << "success: " << x.success;
}

}
