#pragma once

#include <vector>
#include <map>
#include <osmium/handler.hpp>

#include "graph/types.h"
#include "graph/polygon.h"

namespace my::preprocess {

static BgPolygon DEFAULT_POLYGON{};  // by default = no polygon

// libosmium Handler that fills way+nodes structures
struct FillingHandler : public osmium::handler::Handler {
    std::map<WayId, std::vector<LocatedNode> > way_to_nodes;  // stores the nodes of a way
    std::map<NodeOsmId, int> node_use_counter;                // for a given node, counts how many ways use it
    BgPolygon polygon;
    inline FillingHandler(BgPolygon polygon_ = DEFAULT_POLYGON) : polygon(polygon_) {}
    void way(const osmium::Way& way) noexcept;
};

bool is_way_interesting(const osmium::Way& way);
bool is_way_in_polygon(const osmium::Way& way, const BgPolygon& polygon);

}  // namespace my::preprocess
