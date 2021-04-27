#pragma once

#include <istream>
#include "graph/polygon.h"

namespace my::preprocess {

static const std::string NO_POLYGON = "NONE";

BgPolygon get_polygon(std::string polygonfile_path);

}  // namespace my::preprocess
