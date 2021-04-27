#include <iostream>
#include <fstream>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

#include "graph/polygonfile.h"

using namespace std;

namespace my::preprocess {

struct IllFormattedPolygonException : public std::exception {
    IllFormattedPolygonException(string description) : msg{string("Ill-formatted polygon file : ") + description} {}
    const char* what() const throw() { return msg.c_str(); }
    string msg;
};

struct UnreadablePolygonFileException : public std::exception {
    UnreadablePolygonFileException(string filepath) : msg{string("Unable to read polygonefile : ") + filepath} {}
    const char* what() const throw() { return msg.c_str(); }
    string msg;
};

static void assert_geojson_format(bool condition, string description) {
    if (!condition)
        throw IllFormattedPolygonException{description};
}

static vector<pair<double, double>> parse_polygonfile(istream& polygonfile_stream) {
    // EXAMPLE OF POLYGON BUILT WITH https://geojson.io :
    // {
    //   "type": "FeatureCollection",
    //   "features": [
    //     {
    //       "type": "Feature",
    //       "properties": {},
    //       "geometry": {
    //         "type": "Polygon",
    //         "coordinates": [
    //           [
    //             [7.4112653732299805, 43.72955337747962],
    //             [7.410149574279785, 43.72766167132964],
    //             [7.418560981750487, 43.72418821923917],
    //             [7.4260711669921875, 43.73001854197097],
    //             [7.425942420959473, 43.73290248118248],
    //             [7.420578002929687, 43.73284046244549],
    //             [7.4112653732299805, 43.72955337747962]
    //           ]
    //         ]
    //       }
    //     }
    //   ]
    // }

    rapidjson::IStreamWrapper stream_wrapper(polygonfile_stream);
    rapidjson::Document doc;
    doc.ParseStream(stream_wrapper);

    assert_geojson_format(doc.IsObject(), "doc is not an object");
    assert_geojson_format(doc.HasMember("features"), "doc has no 'features'");
    auto& features = doc["features"];

    auto& feature = features[0];
    assert_geojson_format(feature.IsObject(), "features is not an object");
    assert_geojson_format(feature.HasMember("geometry"), "features has no 'geometry'");

    auto& geometry = feature["geometry"];
    assert_geojson_format(geometry.IsObject(), "geometry is not an object");
    assert_geojson_format(geometry.HasMember("type"), "geometry has no 'type'");
    assert_geojson_format(geometry.HasMember("coordinates"), "geometry has no 'coordinates'");

    auto& geom_type = geometry["type"];
    assert_geojson_format(geom_type.IsString(), "type is not a string");
    assert_geojson_format(string(geom_type.GetString()) == "Polygon", "type is not a 'Polygon'");

    auto& coordinates = geometry["coordinates"];
    assert_geojson_format(coordinates.IsArray(), "coordinates is not an Array");

    // it seems that the coordinates format is built to allow multiple polygon ?
    // anyway, we're only interested in the first one :
    assert_geojson_format(coordinates.Size() == 1, "there are multiple polygons");
    auto& first_polygon_coordinates = coordinates[0];
    assert_geojson_format(first_polygon_coordinates.IsArray(), "first polygon coords is not an Array");

    vector<pair<double, double>> points;
    for (auto& coordinate_pair : first_polygon_coordinates.GetArray()) {
        assert_geojson_format(coordinate_pair.IsArray(), "coordinate_pair is not an array");
        assert_geojson_format(coordinate_pair.Size() == 2, "coordinate_pair has not 2 elements");
        auto& lon = coordinate_pair[0];
        auto& lat = coordinate_pair[1];
        assert_geojson_format(lon.IsDouble(), "lon is not a double");
        assert_geojson_format(lat.IsDouble(), "lat is not a double");
        points.emplace_back(lon.GetDouble(), lat.GetDouble());
    }
    return points;
}

BgPolygon get_polygon(string polygonfile_path) {
    // explicitly returning an empty polygon :
    if (polygonfile_path == NO_POLYGON) {
        cerr << "WARNING : no filtering by polygon will be used." << endl;
        return {};
    }

    ifstream polygonfile_stream{polygonfile_path};
    if (!polygonfile_stream.good()) {
        throw UnreadablePolygonFileException{polygonfile_path};
    }

    auto points = parse_polygonfile(polygonfile_stream);
    return create_polygon(points);
}

}  // namespace my::preprocess
