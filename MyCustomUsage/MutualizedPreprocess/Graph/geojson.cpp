#include <fstream>
#include <iomanip>
#include <unordered_map>

#include <rapidjson/istreamwrapper.h>
#include "Graph/geojson.h"

using namespace std;

namespace my {

void dump_geojson_graph(ostream& out, vector<Edge> const& edges, bool allow_unranked) {
    // EXPECTED OUTPUT :
    // {
    //     "type": "FeatureCollection",
    //     "features": [
    //         {
    //             "type": "Feature",
    //             "geometry": {
    //                 "type": "LineString",
    //                 "coordinates": [
    //                     [
    //                         7.4259518,
    //                         43.7389494
    //                     ],
    //                     [
    //                         7.4258602,
    //                         43.7389997
    //                     ]
    //                 ]
    //             },
    //             "properties": {
    //                 "node_from": "https://www.openstreetmap.org/node/21912089",
    //                 "node_to": "https://www.openstreetmap.org/node/7265761724",
    //                 "node_from_url": "https://www.openstreetmap.org/node/21912089",
    //                 "node_to_url": "https://www.openstreetmap.org/node/7265761724",
    //                 "weight": 7.081911563873291,
    //                 "length_meters": 9.245828628540039
    //             }
    //         },
    //         ... other features ...
    //     ]
    // }

    rapidjson::Document doc(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& a = doc.GetAllocator();
    doc.AddMember("type", "FeatureCollection", a);

    rapidjson::Value features(rapidjson::kArrayType);
    for (auto& edge : edges) {
        // coordinates :
        rapidjson::Value coordinates(rapidjson::kArrayType);
        for (auto& node_location : edge.geometry) {
            rapidjson::Value loc(rapidjson::kArrayType);
            loc.PushBack(rapidjson::Value().SetDouble(node_location.lon()), a);
            loc.PushBack(rapidjson::Value().SetDouble(node_location.lat()), a);
            coordinates.PushBack(loc, a);
        }

        // properties :
        rapidjson::Value properties(rapidjson::kObjectType);
        size_t node_from_rank = allow_unranked ? edge.node_from.get_rank_or_unranked() : edge.node_from.get_rank();
        properties.AddMember("node_from_rank", node_from_rank, a);
        properties.AddMember("node_from", rapidjson::Value().SetString(edge.node_from.id.c_str(), a), a);
        size_t node_to_rank = allow_unranked ? edge.node_to.get_rank_or_unranked() : edge.node_to.get_rank();
        properties.AddMember("node_to_rank", node_to_rank, a);
        properties.AddMember("node_to", rapidjson::Value().SetString(edge.node_to.id.c_str(), a), a);
        properties.AddMember("node_from_url", rapidjson::Value().SetString(edge.node_from.url.c_str(), a), a);
        properties.AddMember("node_to_url", rapidjson::Value().SetString(edge.node_to.url.c_str(), a), a);
        properties.AddMember("weight", edge.weight, a);
        properties.AddMember("length_meters", edge.length_m, a);

        // geometry :
        rapidjson::Value geometry(rapidjson::kObjectType);
        geometry.AddMember("type", "LineString", a);
        geometry.AddMember("coordinates", coordinates, a);

        // feature :
        rapidjson::Value feature(rapidjson::kObjectType);
        feature.AddMember("type", "Feature", a);
        feature.AddMember("geometry", geometry, a);
        feature.AddMember("properties", properties, a);
        features.PushBack(feature, a);
    }

    doc.AddMember("features", features, a);

    // dumping :
    rapidjson::OStreamWrapper out_wrapper(out);
    rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(out_wrapper);
    doc.Accept(writer);
}

void dump_geojson_stops(ostream& out, vector<StopWithClosestNode> const& stops) {
    rapidjson::Document doc(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& a = doc.GetAllocator();
    doc.AddMember("type", "FeatureCollection", a);

    rapidjson::Value features(rapidjson::kArrayType);

    for (auto& stop : stops) {
        // coordinates :
        rapidjson::Value coordinates(rapidjson::kArrayType);
        coordinates.PushBack(rapidjson::Value().SetDouble(stop.lon), a);
        coordinates.PushBack(rapidjson::Value().SetDouble(stop.lat), a);

        // geometry :
        rapidjson::Value geometry(rapidjson::kObjectType);
        geometry.AddMember("coordinates", coordinates, a);
        geometry.AddMember("type", "Point", a);

        // properties :
        rapidjson::Value properties(rapidjson::kObjectType);
        properties.AddMember("stop_id", rapidjson::Value().SetString(stop.id.c_str(), a), a);
        properties.AddMember("stop_name", rapidjson::Value().SetString(stop.name.c_str(), a), a);
        properties.AddMember("closest_node_id", rapidjson::Value().SetString(stop.closest_node_id.c_str(), a), a);
        properties.AddMember("closest_node_url", rapidjson::Value().SetString(stop.closest_node_url.c_str(), a), a);

        // feature :
        rapidjson::Value feature(rapidjson::kObjectType);
        feature.AddMember("type", "Feature", a);
        feature.AddMember("geometry", geometry, a);
        feature.AddMember("properties", properties, a);
        features.PushBack(feature, a);
    }

    doc.AddMember("features", features, a);

    // dumping :
    rapidjson::OStreamWrapper out_wrapper(out);
    rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(out_wrapper);
    doc.Accept(writer);
}

struct IllFormattedWalkingGraphException : public exception {
    IllFormattedWalkingGraphException(string description)
        : msg{string("Ill-formatted walking-graph file : ") + description} {}
    const char* what() const throw() { return msg.c_str(); }
    string msg;
};

static void assert_geojson_format(bool condition, string description) {
    if (!condition)
        throw IllFormattedWalkingGraphException{description};
}

vector<Edge> parse_geojson_graph(istream& in) {
    rapidjson::IStreamWrapper stream_wrapper(in);
    rapidjson::Document doc;
    doc.ParseStream(stream_wrapper);

    assert_geojson_format(doc.IsObject(), "doc is not an object");
    assert_geojson_format(doc.HasMember("features"), "doc has no 'features'");
    auto& features = doc["features"];

    assert_geojson_format(features.IsArray(), "features is not an array");

    vector<Edge> edges;
    for (auto ite = features.Begin(); ite != features.End(); ++ite) {
        auto& feature = *ite;
        assert_geojson_format(feature.IsObject(), "features is not an object");
        assert_geojson_format(feature.HasMember("geometry"), "features has no 'geometry'");
        assert_geojson_format(feature.HasMember("type"), "features has no 'type'");
        assert_geojson_format(feature.HasMember("properties"), "features has no 'properties'");

        auto& properties = feature["properties"];
        assert_geojson_format(properties.IsObject(), "properties is not an object");
        assert_geojson_format(properties.HasMember("node_from"), "properties has no 'node_from'");
        assert_geojson_format(properties.HasMember("node_from_rank"), "properties has no 'node_from_rank'");
        assert_geojson_format(properties.HasMember("node_to"), "properties has no 'node_to'");
        assert_geojson_format(properties.HasMember("node_to_rank"), "properties has no 'node_to_rank'");
        assert_geojson_format(properties.HasMember("weight"), "properties has no 'weight'");
        assert_geojson_format(properties.HasMember("length_meters"), "properties has no 'length_meters'");

        auto& geometry = feature["geometry"];
        assert_geojson_format(geometry.IsObject(), "geometry is not an object");
        assert_geojson_format(geometry.HasMember("type"), "geometry has no 'type'");
        assert_geojson_format(geometry.HasMember("coordinates"), "geometry has no 'coordinates'");

        auto& geom_type = geometry["type"];
        assert_geojson_format(geom_type.IsString(), "type is not a string");
        assert_geojson_format(string(geom_type.GetString()) == "LineString", "type is not a 'LineString'");

        auto& coordinates = geometry["coordinates"];
        assert_geojson_format(coordinates.IsArray(), "coordinates is not an Array");

        Polyline polyline;
        for (auto& coordinate_pair : coordinates.GetArray()) {
            assert_geojson_format(coordinate_pair.IsArray(), "coordinate_pair is not an array");
            assert_geojson_format(coordinate_pair.Size() == 2, "coordinate_pair has not 2 elements");
            auto& lon = coordinate_pair[0];
            auto& lat = coordinate_pair[1];
            assert_geojson_format(lon.IsDouble(), "lon is not a double");
            assert_geojson_format(lat.IsDouble(), "lat is not a double");
            polyline.emplace_back(lon.GetDouble(), lat.GetDouble());
        }

        NodeId node_from_id = properties["node_from"].GetString();
        size_t node_from_rank = properties["node_from_rank"].GetUint64();
        NodeId node_to_id = properties["node_to"].GetString();
        size_t node_to_rank = properties["node_to_rank"].GetUint64();
        float length_m = static_cast<float>(properties["length_meters"].GetDouble());
        float weight = static_cast<float>(properties["weight"].GetDouble());

        edges.emplace_back(node_from_id, node_from_rank, node_to_id, node_to_rank, move(polyline), length_m, weight);
    }
    return edges;
}

}  // namespace my
