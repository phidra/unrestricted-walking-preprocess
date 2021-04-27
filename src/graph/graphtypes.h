#pragma once

#include <string>

#include "graph/types.h"

namespace my {

inline std::string node_url(NodeOsmId id) {
    return std::string("https://www.openstreetmap.org/node/") + std::to_string(id);
}
using NodeId = std::string;

struct Node {
    static constexpr const size_t UNRANKED = SIZE_MAX;

    inline Node(NodeOsmId osm_id_, osmium::Location const& location_)
        : url{node_url(osm_id_)},
          id{url},  // for OSM nodes, node ids are their URL
          location{location_} {}

    inline Node(NodeId id, osmium::Location const& location_, size_t rank_ = UNRANKED)
        : url{}, id{id}, location{location_}, rank{rank_} {}

    inline double lon() const { return location.lon(); }
    inline double lat() const { return location.lat(); }

    inline size_t get_rank_or_unranked() const { return rank; }  // this CAN return UNRANKED
    // FIXME : for speed, those checks should be in debug mode :
    inline size_t get_rank() const {
        if (!is_ranked())
            throw std::runtime_error("trying to get rank of UNRANKED node");
        return rank;
    }
    inline void set_rank(size_t rank_) {
        if (is_ranked() && rank_ != rank)
            throw std::runtime_error("trying to set an inconsistent rank");
        rank = rank_;
    }
    inline bool is_ranked() const { return rank != UNRANKED; }

    inline bool operator==(Node const& other) const {
        return this->id == other.id && this->rank == other.rank;
    }  // needed by set<T>

    std::string url;
    NodeId id;
    osmium::Location location;

   private:
    size_t rank = UNRANKED;
};

struct NodeHasher {
    size_t operator()(Node const& n) const { return std::hash<std::string>{}(n.id); }
};

// NOTE : a single OSM way can be splitted in several edges.
struct Edge {
    inline Edge(NodeOsmId node_from_, NodeOsmId node_to_, Polyline&& geometry_, float length_m_, float weight_)
        : node_from{node_from_, geometry_.front()},
          node_to{node_to_, geometry_.back()},
          length_m{length_m_},
          weight{weight_},
          geometry{geometry_} {}

    inline Edge(NodeId node_from_,
                size_t rank_from,
                NodeId node_to_,
                size_t rank_to,
                Polyline&& geometry_,
                float length_m_,
                float weight_)
        : node_from{node_from_, geometry_.front(), rank_from},
          node_to{node_to_, geometry_.back(), rank_to},
          length_m{length_m_},
          weight{weight_},
          geometry{geometry_} {}

    bool operator==(Edge const& other) const {
        return (node_from == other.node_from && node_to == other.node_to && length_m == other.length_m &&
                weight == other.weight && geometry == other.geometry);
    }

    Node node_from;
    Node node_to;
    float length_m;
    float weight;
    Polyline geometry;
};

}  // namespace my
