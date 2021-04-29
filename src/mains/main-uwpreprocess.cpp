#include <iostream>
#include <fstream>
#include <string>

#include "graph/graphtypes.h"
#include "graph/walking_graph.h"
#include "gtfs/gtfs_parsed_data.h"
#include "json/gtfs_serialization.h"
#include "json/walking_graph_serialization.h"
#include "json/polygon_serialization.h"

int main(int argc, char** argv) {
    if (argc < 7) {
        std::cout << "Usage:  " << argv[0]
                  << "  <gtfs_folder>  <osm_file>  <polygon_file>  <walkspeed_km/h>  <output_dir>  <hluw_output_dir>"
                  << std::endl;
        std::exit(0);
    }

    const std::string gtfs_folder = argv[1];
    const std::string osm_file = argv[2];
    const std::string polygon_file = argv[3];
    const float walkspeed_km_per_hr = std::stof(argv[4]);
    std::string output_dir = argv[5];
    if (output_dir.back() != '/') {
        output_dir.push_back('/');
    }

    std::string hluw_output_dir = argv[6];
    if (hluw_output_dir.back() != '/') {
        hluw_output_dir.push_back('/');
    }

    std::cout << "GTFS FOLDER      = " << gtfs_folder << std::endl;
    std::cout << "OSMFILE          = " << osm_file << std::endl;
    std::cout << "POLYGONFILE      = " << polygon_file << std::endl;
    std::cout << "WALKSPEED KM/H   = " << walkspeed_km_per_hr << std::endl;
    std::cout << "OUTPUT_DIR       = " << output_dir << std::endl;
    std::cout << "HL-UW OUTPUT_DIR = " << hluw_output_dir << std::endl;
    std::cout << std::endl;

    // gtfs :
    std::vector<uwpreprocess::Stop> stops;
    {
        std::cout << "Parsing GTFS folder" << std::endl;
        uwpreprocess::GtfsParsedData gtfs_data{gtfs_folder};

        std::cout << "Dumping GTFS as json" << std::endl;
        std::ofstream out_gtfs(output_dir + "gtfs.json");
        uwpreprocess::json::serialize_gtfs(gtfs_data, out_gtfs);

        std::cout << "Dumping HL-UW stoptimes" << std::endl;
        std::ofstream out_stoptimes(hluw_output_dir + "stoptimes.txt");
        gtfs_data.to_hluw_stoptimes(out_stoptimes);

        // note : this conversion is only necessary so that Graph doesn't depend on GtfsParsing :
        std::cout << "Converting stops for walking-graph" << std::endl;
        for (auto& stop : gtfs_data.ranked_stops) {
            stops.emplace_back(stop.longitude, stop.latitude, stop.id, stop.name);
        }

        if (!uwpreprocess::json::_check_serialization_idempotent(gtfs_data)) {
            std::cout << "ERROR - gtfs serialization is not idempotent !" << std::endl;
            return 1;
        }
    }

    // walking-graph :
    std::cout << "Getting polygon" << std::endl;
    uwpreprocess::BgPolygon polygon = uwpreprocess::json::unserialize_polygon(polygon_file);
    std::cout << "Building walking-graph" << std::endl;
    uwpreprocess::WalkingGraph graph{osm_file, polygon, stops, walkspeed_km_per_hr};

    std::cout << "Dumping WalkingGraph for HL-UW" << std::endl;
    uwpreprocess::json::serialize_walking_graph_hluw(graph, hluw_output_dir);

    std::cout << "Dumping WalkingGraph geojson" << std::endl;
    std::ofstream out_graph(output_dir + "walking_graph.json");
    uwpreprocess::json::serialize_walking_graph(graph, out_graph);

    if (!uwpreprocess::json::_check_serialization_idempotent(graph)) {
        std::cout << "ERROR - graph serialization is not idempotent !" << std::endl;
        return 1;
    }

    std::cout << "All is OK" << std::endl;

    return 0;
}
