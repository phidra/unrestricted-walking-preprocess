#pragma once

#include <istream>
#include "graph/polygon.h"

namespace uwpreprocess::json {

static const std::string NO_POLYGON = "NONE";

BgPolygon unserialize_polygon(std::string polygonfile_path);

}  // namespace uwpreprocess
