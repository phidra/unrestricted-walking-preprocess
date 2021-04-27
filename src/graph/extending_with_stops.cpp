#include <boost/geometry.hpp>
#include <osmium/geom/haversine.hpp>

#include "graph/extending_with_stops.h"

using namespace std;

namespace my::preprocess {

using BgDegree = boost::geometry::cs::spherical_equatorial<boost::geometry::degree>;
using BgPoint = boost::geometry::model::point<double, 2, BgDegree>;
using RtreeValue = pair<BgPoint, my::Node>;
using RTree = boost::geometry::index::rtree<RtreeValue, boost::geometry::index::linear<8>>;

RTree index_graph_nodes(vector<my::Edge> const& edgesOsm) {
    RTree rtree;
    for (auto edge : edgesOsm) {
        // the fact that duplicate nodes are inserted doesn't change the final result, as duplicates have the same id.
        rtree.insert(make_pair(BgPoint{edge.node_from.lon(), edge.node_from.lat()}, edge.node_from));
        rtree.insert(make_pair(BgPoint{edge.node_to.lon(), edge.node_to.lat()}, edge.node_to));
    }
    return rtree;
}

my::Node get_closest_node(RTree const& rtree, Stop const& stop) {
    vector<RtreeValue> closest_nodes;
    BgPoint stoppoint{stop.lon, stop.lat};
    rtree.query(boost::geometry::index::nearest(stoppoint, 1), back_inserter(closest_nodes));
    my::Node closest_node = closest_nodes.front().second;
    return closest_node;
}

pair<vector<my::Edge>, vector<my::StopWithClosestNode>> extend_graph(vector<Stop> const& stops,
                                                                     vector<my::Edge> const& edgesOsm,
                                                                     float walkspeed_km_per_h) {
    // note : this is currently done in multiple steps (+ copies) for code clarity
    //        but if performance is an issue, we could easily do better

    // index all nodes in graph :
    RTree rtree = index_graph_nodes(edgesOsm);

    // for each stop, find closest node in graph, and adds an edge from stop to closest node :
    vector<my::Edge> edgesExtendedWithStops = edgesOsm;
    vector<my::StopWithClosestNode> stops_with_closest_node;
    for (auto& stop : stops) {
        my::Node closest_node = get_closest_node(rtree, stop);

        // we now extend graph with a straight edge from stop to closest node :
        my::Polyline geometry{{stop.lon, stop.lat}, {closest_node.lon(), closest_node.lat()}};
        float distance_in_meters = osmium::geom::haversine::distance(geometry.front(), geometry.back());
        auto walkspeed_m_per_second = walkspeed_km_per_h * 1000 / 3600;
        float weight_in_seconds = distance_in_meters / walkspeed_m_per_second;
        edgesExtendedWithStops.emplace_back(stop.id, Node::UNRANKED, closest_node.id, Node::UNRANKED,
                                            std::move(geometry), distance_in_meters, weight_in_seconds);

        // for each stop, we memorize the closest node :
        stops_with_closest_node.emplace_back(stop, closest_node.id, closest_node.url);
    }
    return {edgesExtendedWithStops, stops_with_closest_node};
}

}  // namespace my::preprocess
