#include <iostream>
#include <osmium/osm/way.hpp>

#include "graph/osmparsing.h"

using namespace std;

namespace uwpreprocess {

void FillingHandler::way(const osmium::Way& way) noexcept {
    if (!is_way_interesting(way))
        return;
    if (!is_way_in_polygon(way, polygon))
        return;

    vector<LocatedNode> nodes;
    for (auto const& node : way.nodes()) {
        auto const& loc = node.location();
        if (!loc.valid()) {
            cout << "NOT IMPLEMENTED: can't handle node with invalid location : " << way.id() << endl;
            exit(42);
        }

        // simply fills data tructures :
        nodes.emplace_back(node.ref(), node.location());
        if (node_use_counter.find(node.ref()) == node_use_counter.end()) {
            node_use_counter.emplace(node.ref(), 0);
        }
        ++node_use_counter[node.ref()];
    }
    way_to_nodes.emplace(way.id(), move(nodes));
};

bool is_way_interesting(const osmium::Way& way) {
    // as a rule of thumb, if a way has the 'highway' tag, it can be used for routing :
    // FIXME : we would probably like to filter out non-pedestrian ways.
    if (way.tags()["highway"] == nullptr)
        return false;

    // some ways aren't a road, but delimits an area. We aren't interesting in them :
    if (way.tags()["area"] != nullptr && strcmp(way.tags()["area"], "yes") == 0)
        return false;

    // way is too short :
    if (way.nodes().size() < 2)
        return false;

    return true;
}

bool is_way_in_polygon(const osmium::Way& way, const BgPolygon& polygon) {
    // if there is no polygon, consider that all ways are ok :
    if (uwpreprocess::is_empty(polygon)) {
        return true;
    }

    // otherwise, a way is considered in polygon if any of its extremities is inside the polygon :
    auto front = way.nodes().front();
    auto back = way.nodes().front();
    bool is_front_in_polygon = is_inside(polygon, front.lon(), front.lat());
    bool is_back_in_polygon = is_inside(polygon, back.lon(), back.lat());
    return is_front_in_polygon || is_back_in_polygon;
}

}  // namespace uwpreprocess
