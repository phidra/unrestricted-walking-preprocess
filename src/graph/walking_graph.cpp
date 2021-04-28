#include <iostream>
#include <fstream>
#include <map>

#include "graph/walking_graph.h"
#include "graph/polygonfile.h"
#include "graph/extending_with_stops.h"
#include "graph/graph.h"
#include "graph/geojson.h"

using namespace std;

namespace uwpreprocess {

void _rank_nodes(vector<uwpreprocess::Edge>& edges_with_stops, vector<uwpreprocess::Stop> const& stops) {
    unordered_map<uwpreprocess::NodeId, size_t> node_to_rank;

    // some algorithms (ULTRA) require that stops are the first nodes of the graph -> we rank stops first :
    size_t current_rank = 0;
    for_each(stops.cbegin(), stops.cend(), [&node_to_rank, &current_rank](uwpreprocess::Stop const& stop) {
        node_to_rank.insert({stop.id, current_rank++});
    });

    auto rank_that_node = [&node_to_rank, &current_rank](auto& node) {
        if (node_to_rank.find(node.id) == node_to_rank.end()) {
            node_to_rank.insert({node.id, current_rank++});
        }
        node.set_rank(node_to_rank.at(node.id));
    };

    // then we can rank the other nodes in the graph :
    for (auto& edge : edges_with_stops) {
        rank_that_node(edge.node_from);
        rank_that_node(edge.node_to);
    }
}

vector<uwpreprocess::Edge> _add_reversed_edges(vector<uwpreprocess::Edge> const& edges) {
    // For each edge, adds its reversed edge (this doubles the number of edges in the edgelist)
    // note : if performance issues arise, the function would be faster with side-effects (mutating the edges)
    // it is kept as is for now, as it is cleaner.
    vector<uwpreprocess::Edge> bidirectional(edges);
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

map<size_t, vector<size_t>> _map_nodes_to_out_edges(vector<uwpreprocess::Edge> const& edges) {
    // this functions build a map that helps to retrieve the out-edges of a node (given its rank)
    map<size_t, vector<size_t>> node_to_out_edges;
    for (size_t edge_index = 0; edge_index < edges.size(); ++edge_index) {
        auto const& edge = edges[edge_index];
        node_to_out_edges[edge.node_from.get_rank()].push_back(edge_index);
    }
    return node_to_out_edges;
}

WalkingGraph::WalkingGraph(filesystem::path osm_file,
                           filesystem::path polygon_file,
                           vector<uwpreprocess::Stop> const& stops,
                           float walkspeed_km_per_hour_)
    : walkspeed_km_per_hour{walkspeed_km_per_hour_},
      polygon{get_polygon(polygon_file)},
      edges_osm{osm_to_graph(osm_file, polygon, walkspeed_km_per_hour)} {
    // extend graph with stop-edges :
    tie(edges_with_stops, stops_with_closest_node) = extend_graph(stops, edges_osm, walkspeed_km_per_hour);

    _rank_nodes(edges_with_stops, stops);
    edges_with_stops_bidirectional = _add_reversed_edges(edges_with_stops);
    node_to_out_edges = _map_nodes_to_out_edges(edges_with_stops_bidirectional);
    cout << "Number of nodes in the graph = " << node_to_out_edges.size() << endl;
    cout << "Number of edges in the graph = " << edges_with_stops_bidirectional.size() << endl;
    check_structures_consistency();
}

void WalkingGraph::dump_intermediary(string const& output_dir) const {
    ofstream original_graph_stream(output_dir + "original_graph.geojson");
    uwpreprocess::dump_geojson_graph(original_graph_stream, edges_osm, true);

    ofstream extended_graph_stream(output_dir + "graph_with_stops.geojson");
    uwpreprocess::dump_geojson_graph(extended_graph_stream, edges_with_stops, true);

    ofstream stops_stream(output_dir + "stops.geojson");
    uwpreprocess::dump_geojson_stops(stops_stream, stops_with_closest_node);

    ofstream polygon_stream(output_dir + "polygon.geojson");
    uwpreprocess::dump_geojson_line(polygon_stream, polygon.outer());
}

void WalkingGraph::print_stats(ostream& out) const {
    out << "Number of edges in original graph : " << this->edges_osm.size() << endl;
    out << "nb edges (including added stops) = " << this->edges_with_stops.size() << endl;
    out << "nb stops = " << this->stops_with_closest_node.size() << endl;
}

void WalkingGraph::check_structures_consistency() const {
    // check structures consistency :
    //  - each node in the node-structure are used in at least one edge
    //  - each edge's extremities in the edge-structure exists in the node-strcture
    // FIXME : this should be used in debug settings only.
    set<size_t> nodes1;
    for (auto& edge : edges_with_stops_bidirectional) {
        nodes1.insert(edge.node_from.get_rank());
        nodes1.insert(edge.node_to.get_rank());
    }

    set<size_t> nodes2;
    for (auto& [rank, _] : node_to_out_edges) {
        nodes2.insert(rank);
    }

    if (nodes1 != nodes2) {
        cout << "ERROR : structures inconsistency :" << endl;
        cout << "nodes1 (used in vector<Edge>) is of size = " << nodes1.size() << endl;
        cout << "nodes2 (used in node_to_out_edges) is of size = " << nodes2.size() << endl;
        exit(1);
    }
}

void WalkingGraph::to_stream(ostream& out) const {
    dump_geojson_graph(out, edges_with_stops_bidirectional, false);
}

void WalkingGraph::to_hluw_structures(std::string const& hluw_output_dir) const {
    // this functions dumps the structures used by HL-UW.
    // FIXME : it should rather be in HL-UW repository (but for now, it is easier to have it there).

    // walkspeed :
    std::ofstream out_walkspeed(hluw_output_dir + "walkspeed_km_per_hour.txt");
    out_walkspeed << walkspeed_km_per_hour << "\n";

    // edges :
    std::ofstream out_edges(hluw_output_dir + "graph.edgefile");
    out_edges << std::fixed << std::setprecision(0);  // displays integer weight
    for (auto& edge : edges_with_stops_bidirectional) {
        out_edges << edge.node_from.id << " ";
        out_edges << edge.node_to.id << " ";
        out_edges << edge.weight << "\n";
    }

    // nodes :
    std::ofstream out_nodes(hluw_output_dir + "stops.nodes");
    for (auto& stop : stops_with_closest_node) {
        out_nodes << stop.id << "\n";
    }

    // stops geojson (used by the HL-UW server) :
    std::ofstream out_stops(hluw_output_dir + "stops.geojson");
    dump_geojson_stops(out_stops, stops_with_closest_node);
}

WalkingGraph WalkingGraph::from_stream(istream& in) {
    WalkingGraph deserialized;
    deserialized.edges_with_stops_bidirectional = parse_geojson_graph(in);

    map<size_t, vector<size_t>> node_to_out_edges;
    size_t edge_rank = 0;
    for (auto& edge : deserialized.edges_with_stops_bidirectional) {
        deserialized.node_to_out_edges[edge.node_from.get_rank()].push_back(edge_rank++);
    }
    deserialized.check_structures_consistency();
    return deserialized;
}

}  // namespace uwpreprocess
