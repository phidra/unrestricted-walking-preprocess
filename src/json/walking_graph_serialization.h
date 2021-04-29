#pragma once

#include <vector>
#include <ostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>

#include "graph/types.h"
#include "graph/polygon.h"
#include "graph/graphtypes.h"
#include "graph/walking_graph.h"

namespace uwpreprocess::json {

void dump_geojson_graph(std::ostream& out, std::vector<Edge> const& edges, bool allow_unranked);
void dump_geojson_stops(std::ostream& out, std::vector<StopWithClosestNode> const& stops);
std::vector<Edge> parse_geojson_graph(std::istream& in);
void dump_geojson_line(std::ostream& out, BgPolygon::ring_type const&);

void serialize_walking_graph(WalkingGraph const&, std::ostream& out);
WalkingGraph unserialize_walking_graph(std::istream& in);
void serialize_walking_graph_hluw(WalkingGraph const&, std::string const& hluw_output_dir);  // FIXME : this should be in HL-UW repo

}  // namespace uwpreprocess
