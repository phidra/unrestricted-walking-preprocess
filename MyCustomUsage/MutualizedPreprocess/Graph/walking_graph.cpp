#include <iostream>
#include <fstream>
#include <map>

#include "Graph/walking_graph.h"
#include "Graph/polygonfile.h"
#include "Graph/extending_with_stops.h"
#include "Graph/graph.h"
#include "Graph/geojson.h"

using namespace std;

namespace my::preprocess {

void _rankNodes(vector<my::Edge>& edgesWithStops, vector<my::Stop> const& stops) {
    unordered_map<my::NodeId, size_t> nodeToRank;

    // some algorithms (ULTRA) require that stops are the first nodes of the graph -> we rank stops first :
    size_t current_rank = 0;
    for_each(stops.cbegin(), stops.cend(), [&nodeToRank, &current_rank](my::Stop const& stop) {
        nodeToRank.insert({stop.id, current_rank++});
    });

    auto rankThatNode = [&nodeToRank, &current_rank](auto& node) {
        if (nodeToRank.find(node.id) == nodeToRank.end()) {
            nodeToRank.insert({node.id, current_rank++});
        }
        node.set_rank(nodeToRank.at(node.id));
    };

    // then we can rank the other nodes in the graph :
    for (auto& edge : edgesWithStops) {
        rankThatNode(edge.node_from);
        rankThatNode(edge.node_to);
    }
}

vector<my::Edge> _makeEdgesBidirectional(vector<my::Edge> const& edges) {
    // For each edge, adds its opposite edge (this doubles the number of edges in the edgelist)
    // note : if performance issues arise, the function would be faster with side-effects (mutating the edges)
    // it is kept as is for now, as it is cleaner.
    vector<my::Edge> bidirectional(edges);
    for (auto edge : edges) {
        Polyline reversed_geom = edge.geometry;
        reverse(reversed_geom.begin(), reversed_geom.end());
        bidirectional.emplace_back(edge.node_to.id, edge.node_to.get_rank(), edge.node_from.id,
                                   edge.node_from.get_rank(), move(reversed_geom), edge.length_m, edge.weight);
    }
    assert(bidirectional.size() == 2 * edges.size());

    // FIXME : here, add a (debug only) check that the nodes of the graph are unchanged by this function
    return bidirectional;
}

map<size_t, vector<size_t>> _mapNodesToOutEdges(vector<my::Edge> const& edges) {
    // this functions build a map that helps to retrieve the out-edges of a node (given its rank)
    map<size_t, vector<size_t>> nodeToOutEdges;
    for (size_t edge_index = 0; edge_index < edges.size(); ++edge_index) {
        auto const& edge = edges[edge_index];
        nodeToOutEdges[edge.node_from.get_rank()].push_back(edge_index);
    }
    return nodeToOutEdges;
}

WalkingGraph::WalkingGraph(filesystem::path osmFile,
                           filesystem::path polygonFile,
                           vector<my::Stop> const& stops,
                           float walkspeedKmPerHour_)
    : walkspeedKmPerHour{walkspeedKmPerHour_},
      polygon{get_polygon(polygonFile)},
      edgesOsm{osm_to_graph(osmFile, polygon, walkspeedKmPerHour)} {
    // extend graph with stop-edges :
    tie(edgesWithStops, stopsWithClosestNode) = extend_graph(stops, edgesOsm, walkspeedKmPerHour);

    _rankNodes(edgesWithStops, stops);
    edgesWithStopsBidirectional = _makeEdgesBidirectional(edgesWithStops);
    nodeToOutEdges = _mapNodesToOutEdges(edgesWithStopsBidirectional);
    cout << "Number of nodes in the graph = " << nodeToOutEdges.size() << endl;
    cout << "Number of edges in the graph = " << edgesWithStopsBidirectional.size() << endl;
    checkStructuresConsistency();
}

void WalkingGraph::dumpIntermediary(string const& outputDir) const {
    ofstream originalGraphStream(outputDir + "original_graph.geojson");
    my::dump_geojson_graph(originalGraphStream, edgesOsm, true);

    ofstream extendedGraphStream(outputDir + "graph_with_stops.geojson");
    my::dump_geojson_graph(extendedGraphStream, edgesWithStops, true);

    ofstream stopsStream(outputDir + "stops.geojson");
    my::dump_geojson_stops(stopsStream, stopsWithClosestNode);

    ofstream polygonStream(outputDir + "polygon.geojson");
    my::dump_geojson_line(polygonStream, polygon.outer());
}

void WalkingGraph::printStats(ostream& out) const {
    out << "Number of edges in original graph : " << this->edgesOsm.size() << endl;
    out << "nb edges (including added stops) = " << this->edgesWithStops.size() << endl;
    out << "nb stops = " << this->stopsWithClosestNode.size() << endl;
}

void WalkingGraph::checkStructuresConsistency() const {
    // check structures consistency :
    //  - each node in the node-structure are used in at least one edge
    //  - each edge's extremities in the edge-structure exists in the node-strcture
    // FIXME : this should be used in debug settings only.
    set<size_t> nodes1;
    for (auto& edge : edgesWithStopsBidirectional) {
        nodes1.insert(edge.node_from.get_rank());
        nodes1.insert(edge.node_to.get_rank());
    }

    set<size_t> nodes2;
    for (auto& [rank, _] : nodeToOutEdges) {
        nodes2.insert(rank);
    }

    if (nodes1 != nodes2) {
        cout << "ERROR : structures inconsistency :" << endl;
        cout << "nodes1 (used in vector<Edge>) is of size = " << nodes1.size() << endl;
        cout << "nodes2 (used in nodeToOutEdges) is of size = " << nodes2.size() << endl;
        exit(1);
    }
}

void WalkingGraph::toStream(ostream& out) const {
    dump_geojson_graph(out, edgesWithStopsBidirectional, false);
}

void WalkingGraph::toHluwStructures(std::string const& hluwOutputDir) const {
    // this functions dumps the structures used by HL-UW.
    // FIXME : it should rather be in HL-UW repository (but for now, it is easier to have it there).

    // walkspeed :
    std::ofstream out_walkspeed(hluwOutputDir + "walkspeed_km_per_hour.txt");
    out_walkspeed << walkspeedKmPerHour << "\n";

    // edges :
    std::ofstream out_edges(hluwOutputDir + "graph.edgefile");
    out_edges << std::fixed << std::setprecision(0);  // displays integer weight
    for (auto& edge : edgesWithStopsBidirectional) {
        out_edges << edge.node_from.id << " ";
        out_edges << edge.node_to.id << " ";
        out_edges << edge.weight << "\n";
    }

    // nodes :
    std::ofstream out_nodes(hluwOutputDir + "stops.nodes");
    for (auto& stop : stopsWithClosestNode) {
        out_nodes << stop.id << "\n";
    }

    // stops geojson (used by the HL-UW server) :
    std::ofstream out_stops(hluwOutputDir + "stops.geojson");
    dump_geojson_stops(out_stops, stopsWithClosestNode);
}

WalkingGraph WalkingGraph::fromStream(istream& in) {
    WalkingGraph deserialized;
    deserialized.edgesWithStopsBidirectional = parse_geojson_graph(in);

    map<size_t, vector<size_t>> nodeToOutEdges;
    size_t edge_rank = 0;
    for (auto& edge : deserialized.edgesWithStopsBidirectional) {
        deserialized.nodeToOutEdges[edge.node_from.get_rank()].push_back(edge_rank++);
    }
    deserialized.checkStructuresConsistency();
    return deserialized;
}

}  // namespace my::preprocess
