#pragma once

#include <vector>
#include <ostream>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/ostreamwrapper.h>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>

#include "graph/types.h"
#include "graph/graphtypes.h"

namespace my {

void dump_geojson_graph(std::ostream& out, std::vector<Edge> const& edges, bool allow_unranked);
void dump_geojson_stops(std::ostream& out, std::vector<StopWithClosestNode> const& stops);
std::vector<Edge> parse_geojson_graph(std::istream& in);

template <typename PointsOfLine>
void dump_geojson_line(std::ostream& out, PointsOfLine const& pointsOfLine) {
    rapidjson::Document doc(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& a = doc.GetAllocator();
    doc.AddMember("type", "FeatureCollection", a);

    rapidjson::Value features(rapidjson::kArrayType);

    // coordinates :
    rapidjson::Value coordinates(rapidjson::kArrayType);

    for (auto point = boost::begin(pointsOfLine); point != boost::end(pointsOfLine); ++point) {
        double lng = boost::geometry::get<0>(*point);
        double lat = boost::geometry::get<1>(*point);
        rapidjson::Value coords(rapidjson::kArrayType);
        coords.PushBack(rapidjson::Value(lng), a);
        coords.PushBack(rapidjson::Value(lat), a);
        coordinates.PushBack(coords, a);
    }

    // geometry :
    rapidjson::Value geometry(rapidjson::kObjectType);
    geometry.AddMember("coordinates", coordinates, a);
    geometry.AddMember("type", "LineString", a);

    // properties :
    rapidjson::Value properties(rapidjson::kObjectType);
    properties.AddMember("nb_points", rapidjson::Value(boost::geometry::num_points(pointsOfLine)), a);

    // feature :
    rapidjson::Value feature(rapidjson::kObjectType);
    feature.AddMember("type", "Feature", a);
    feature.AddMember("geometry", geometry, a);
    feature.AddMember("properties", properties, a);
    features.PushBack(feature, a);

    doc.AddMember("features", features, a);

    // dumping :
    rapidjson::OStreamWrapper out_wrapper(out);
    rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(out_wrapper);
    doc.Accept(writer);

    out << std::endl;
}

}  // namespace my
