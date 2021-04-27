#pragma once

#include <vector>
#include <filesystem>

#include "graph/graphtypes.h"
#include "graph/polygon.h"

namespace my::preprocess {

struct WalkingGraph {
    // From a set of stops and a given OSM file (+ a possible filtering polygon), computes a walking graph.
    //
    // The graph is extended with new nodes (the stops) and new edges (between each stop and its closest OSM node).
    // Also : the edges are duplicated to make the graph bidirectional.
    //
    // Each node of the graph is identified by its rank.
    // Nodes representing stops are ranked BEFORE the other nodes (because this is needed by ULTRA).

    WalkingGraph(std::filesystem::path osmFile,
                 std::filesystem::path polygonFile,
                 std::vector<my::Stop> const& stops,
                 float walkspeedKmPerHour_);

    WalkingGraph(WalkingGraph&&) = default;
    WalkingGraph() {}
    //
    // edges3 = same than edges2, but twice as more because bidirectional :
    std::vector<my::Edge> edgesWithStopsBidirectional;

    // helper structures :
    std::map<size_t, std::vector<size_t>> nodeToOutEdges;

    // this dump helper is currently used to check non-regression (output must be binary iso)
    // later, we can remove it or replace it with proper tests, but for now it is useful
    void dumpIntermediary(std::string const& outputDir) const;

    void printStats(std::ostream& out) const;

    void checkStructuresConsistency() const;

    // serialization/deserialization :
    void toStream(std::ostream& out) const;
    static WalkingGraph fromStream(std::istream& in);
    void toHluwStructures(std::string const& hluwOutputDir) const;  // FIXME : this should be in HL-UW repo

   private:
    float walkspeedKmPerHour;
    my::BgPolygon polygon;

    // edges1 = those are the "initial" edges, in the OSM graph :
    std::vector<my::Edge> edgesOsm;

    // edges2 = those are the edges "augmented" with an edge between each stop and its closest initial node :
    std::vector<my::Edge> edgesWithStops;

    // those are the stops passed as parameters, augmented with their closest node in the OSM graph :
    std::vector<my::StopWithClosestNode> stopsWithClosestNode;
};

}  // namespace my::preprocess
