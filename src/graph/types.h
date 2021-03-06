#pragma once

#include <unordered_map>
#include <vector>
#include <string>

#include <osmium/osm/location.hpp>
#include <osmium/osm/types.hpp>

namespace uwpreprocess {

using NodeOsmId = osmium::object_id_type;
using WayId = osmium::object_id_type;
using LocatedNode = std::pair<NodeOsmId, osmium::Location>;
using Polyline = std::vector<osmium::Location>;
using StopId = std::string;

struct Stop {
    inline Stop(double lon_, double lat_, StopId id_, std::string name_) : lon(lon_), lat(lat_), id{id_}, name{name_} {}
    inline Stop(Stop const& stop) : lon(stop.lon), lat(stop.lat), id{stop.id}, name{stop.name} {}
    double lon;
    double lat;
    StopId id;
    std::string name;

    bool operator==(Stop const& x) const { return lat == x.lat && lon == x.lon && id == x.id && name == x.name; }
};

struct StopWithClosestNode : public Stop {
    inline StopWithClosestNode(Stop const& stop_,
                               std::string const& closest_node_id_,
                               std::string const& closest_node_url_)
        : Stop{stop_}, closest_node_id{closest_node_id_}, closest_node_url{closest_node_url_} {}

    std::string closest_node_id;
    std::string closest_node_url;

    bool operator==(StopWithClosestNode const& x) const { return static_cast<Stop const&>(*this) == static_cast<Stop const&>(x) && closest_node_id == x.closest_node_id && closest_node_url == x.closest_node_url; }
};

}  // namespace uwpreprocess
