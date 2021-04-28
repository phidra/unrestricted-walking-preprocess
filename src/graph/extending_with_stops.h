#pragma once

#include <vector>

#include "graph/types.h"
#include "graph/graphtypes.h"

namespace uwpreprocess {

std::pair<std::vector<Edge>, std::vector<StopWithClosestNode>> extend_graph(std::vector<Stop> const& stops,
                                                                            std::vector<Edge> const& edges_osm,
                                                                            float walkspeed_km_per_h);

}
