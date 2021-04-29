#pragma once

#include <vector>
#include <filesystem>

#include "graph/graphtypes.h"
#include "graph/polygon.h"

namespace uwpreprocess {

struct WalkingGraph {
    // From a set of stops and a given OSM file (+ a possible filtering polygon), computes a walking graph.
    //
    // The graph is extended with new nodes (the stops) and new edges (between each stop and its closest OSM node).
    // Also : the edges are duplicated to make the graph bidirectional.
    //
    // Each node of the graph is identified by its rank.
    // Nodes representing stops are ranked BEFORE the other nodes (because this is needed by ULTRA).

    WalkingGraph(std::filesystem::path osm_file,
                 BgPolygon polygon_,
                 std::vector<uwpreprocess::Stop> const& stops,
                 float walkspeed_km_per_hour_);

    WalkingGraph(WalkingGraph&&) = default;
    WalkingGraph() {}

    void check_structures_consistency() const;

    // edges in graph OSM + an additional edge for each stops + all edges are duplicated to make them bidirectional :
    std::vector<uwpreprocess::Edge> edges_with_stops_bidirectional;

    // helper structures :
    std::vector<std::vector<size_t>> node_to_out_edges;

    float walkspeed_km_per_hour;
    uwpreprocess::BgPolygon polygon;

    // those are the stops passed as parameters, augmented with their closest node in the OSM graph :
    std::vector<uwpreprocess::StopWithClosestNode> stops_with_closest_node;
};

}  // namespace uwpreprocess
