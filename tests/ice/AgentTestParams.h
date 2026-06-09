#pragma once

#include "NatEmulator.h"
#include <etl/string_stream.h>

namespace tau::ice {

struct AgentTestParams {
    NatEmulator::Type peer1_nat_type;
    size_t peer1_sockets_count;
    bool peer1_has_turn;

    NatEmulator::Type peer2_nat_type;
    size_t peer2_sockets_count;
    bool peer2_has_turn;

    bool nominating_strategy_best;

    bool success;
};

inline etl::string_stream& operator<<(etl::string_stream& ss, const AgentTestParams& x) {
    return ss
        << "peer1: {nat: " << x.peer1_nat_type << ", sockets: " << x.peer1_sockets_count << ", turn: " << x.peer1_has_turn << "}, "
        << "peer2: {nat: " << x.peer2_nat_type << ", sockets: " << x.peer2_sockets_count << ", turn: " << x.peer2_has_turn << "}, "
        << "nominating_strategy_best: " << x.nominating_strategy_best << ", "
        << "success: " << x.success;
}

}
