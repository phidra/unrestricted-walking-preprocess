#pragma once

#include <vector>
#include <ostream>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/ostreamwrapper.h>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>

#include "graph/types.h"
#include "graph/polygon.h"
#include "graph/graphtypes.h"

namespace my {

void dump_geojson_graph(std::ostream& out, std::vector<Edge> const& edges, bool allow_unranked);
void dump_geojson_stops(std::ostream& out, std::vector<StopWithClosestNode> const& stops);
std::vector<Edge> parse_geojson_graph(std::istream& in);
void dump_geojson_line(std::ostream& out, BgPolygon::ring_type const&);

}  // namespace my
