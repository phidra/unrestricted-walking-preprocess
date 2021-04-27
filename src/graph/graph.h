#pragma once

#include <vector>

#include "graph/graphtypes.h"
#include "graph/polygon.h"

namespace my::preprocess {

std::vector<Edge> osm_to_graph(std::string osmfile, BgPolygon polygon, float walkspeed_km_per_h);

}
