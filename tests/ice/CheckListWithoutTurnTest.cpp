#include "tests/ice/CheckListTest.h"

namespace tau::ice {

std::vector<CheckListTestParams> MakeCheckListParamsWithoutTurn() {
    const std::vector<NatEmulator::Type> nat_types = {
        NatEmulator::Type::kFullCone,
        NatEmulator::Type::kRestrictedCone,
        NatEmulator::Type::kPortRestrictedCone,
        NatEmulator::Type::kSymmetric,
        // NatEmulator::Type::kLocalNetworkOnly,
    };

    std::vector<CheckListTestParams> params;

    for(size_t i = 0; i < nat_types.size(); ++i) {
        for(size_t j = i; j < nat_types.size(); ++j) {
            bool success = true;
            if(nat_types[i] == NatEmulator::Type::kSymmetric) {
                success = false;
            }
            if(nat_types[i] == NatEmulator::Type::kPortRestrictedCone) {
                if(nat_types[j] == NatEmulator::Type::kSymmetric) {
                    success = false;
                }
            }

            for(size_t s1 = 1; s1 <= 3; ++s1) {
            for(size_t s2 = 1; s2 <= 2; ++s2) {
                params.push_back(CheckListTestParams{
                    .peer1_nat_type = nat_types[i],
                    .peer1_sockets_count = s1,
                    .peer2_nat_type = nat_types[j],
                    .peer2_sockets_count = s2,
                    .success = success
                });
                params.push_back(CheckListTestParams{
                    .peer1_nat_type = nat_types[j],
                    .peer1_sockets_count = s1,
                    .peer2_nat_type = nat_types[i],
                    .peer2_sockets_count = s2,
                    .success = success
                });
            }}
        }
    }
    return params;
}

INSTANTIATE_TEST_SUITE_P(WithoutTurn, CheckListTest, ::testing::ValuesIn(MakeCheckListParamsWithoutTurn()));

}
