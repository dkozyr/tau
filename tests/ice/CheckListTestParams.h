#pragma once

#include "tests/ice/NatEmulator.h"

namespace tau::ice {

struct CheckListTestParams {
    NatEmulator::Type peer1_nat_type;
    size_t peer1_sockets_count;

    NatEmulator::Type peer2_nat_type;
    size_t peer2_sockets_count;

    bool success;
};

inline std::ostream& operator<<(std::ostream& s, const CheckListTestParams& x) {
    return s
        << "peer1: {nat: " << x.peer1_nat_type << ", sockets: " << x.peer1_sockets_count << "}, "
        << "peer2: {nat: " << x.peer2_nat_type << ", sockets: " << x.peer2_sockets_count << "}, "
        << "success: " << x.success;
}

}
