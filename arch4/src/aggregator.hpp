#pragma once

#include "common.hpp"
#include "router.hpp"

namespace arch4 {

struct AggregatedAnswer {
    std::string text;
    std::vector<std::pair<std::string,double>> provenance; // (patch_id, weight)
};

class Aggregator {
public:
    AggregatedAnswer aggregate(const std::vector<std::pair<PatchRecord,std::string>> &outputs) const;
};

}

