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
                 std::filesystem::path polygon_file,
                 std::vector<uwpreprocess::Stop> const& stops,
                 float walkspeed_km_per_hour_);

    WalkingGraph(WalkingGraph&&) = default;
    WalkingGraph() {}
    //
    // edges3 = same than edges2, but twice as more because bidirectional :
    std::vector<uwpreprocess::Edge> edges_with_stops_bidirectional;

    // helper structures :
    std::map<size_t, std::vector<size_t>> node_to_out_edges;

    // this dump helper is currently used to check non-regression (output must be binary iso)
    // later, we can remove it or replace it with proper tests, but for now it is useful
    void dump_intermediary(std::string const& output_dir) const;

    void print_stats(std::ostream& out) const;

    void check_structures_consistency() const;

    // serialization/deserialization :
    void to_stream(std::ostream& out) const;
    static WalkingGraph from_stream(std::istream& in);
    void to_hluw_structures(std::string const& hluw_output_dir) const;  // FIXME : this should be in HL-UW repo

   private:
    float walkspeed_km_per_hour;
    uwpreprocess::BgPolygon polygon;

    // edges1 = those are the "initial" edges, in the OSM graph :
    std::vector<uwpreprocess::Edge> edges_osm;

    // edges2 = those are the edges "augmented" with an edge between each stop and its closest initial node :
    std::vector<uwpreprocess::Edge> edges_with_stops;

    // those are the stops passed as parameters, augmented with their closest node in the OSM graph :
    std::vector<uwpreprocess::StopWithClosestNode> stops_with_closest_node;
};

}  // namespace uwpreprocess
