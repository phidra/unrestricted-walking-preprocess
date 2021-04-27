#pragma once

#include <vector>

#include "Graph/graphtypes.h"
#include "Graph/polygon.h"

namespace my::preprocess {

std::vector<Edge> osm_to_graph(std::string osmfile, BgPolygon polygon, float walkspeed_km_per_h);

}
