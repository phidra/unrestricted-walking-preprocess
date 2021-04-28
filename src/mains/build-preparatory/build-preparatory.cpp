#include <iostream>
#include <fstream>
#include <string>

#include "graph/graphtypes.h"
#include "graph/walking_graph.h"
#include "gtfs/gtfs_parsed_data.h"
#include "gtfs/gtfs_geojson.h"

int main(int argc, char** argv) {
    if (argc < 7) {
        std::cout << "Usage:  " << argv[0]
                  << "  <GTFS folder>  <osmFile>  <polygonFile>  <walkspeed km/h>  <outputDir>  <hluwOutputDir>"
                  << std::endl;
        std::exit(0);
    }

    const std::string gtfsFolder = argv[1];
    const std::string osmFile = argv[2];
    const std::string polygonFile = argv[3];
    const float walkspeedKmPerHour = std::stof(argv[4]);
    std::string outputDir = argv[5];
    if (outputDir.back() != '/') {
        outputDir.push_back('/');
    }

    std::string hluwOutputDir = argv[6];
    if (hluwOutputDir.back() != '/') {
        hluwOutputDir.push_back('/');
    }

    std::cout << "GTFS FOLDER      = " << gtfsFolder << std::endl;
    std::cout << "OSMFILE          = " << osmFile << std::endl;
    std::cout << "POLYGONFILE      = " << polygonFile << std::endl;
    std::cout << "WALKSPEED KM/H   = " << walkspeedKmPerHour << std::endl;
    std::cout << "OUTPUT_DIR       = " << outputDir << std::endl;
    std::cout << "HL-UW OUTPUT_DIR = " << hluwOutputDir << std::endl;
    std::cout << std::endl;

    // gtfs :
    std::vector<my::Stop> stops;
    {
        std::cout << "Parsing GTFS folder" << std::endl;
        my::preprocess::GtfsParsedData gtfsData{gtfsFolder};

        std::cout << "Dumping GTFS as json" << std::endl;
        std::ofstream out_gtfs(outputDir + "gtfs.json");
        my::preprocess::toStream(out_gtfs, gtfsData);

        std::cout << "Dumping HL-UW stoptimes" << std::endl;
        std::ofstream out_stoptimes(hluwOutputDir + "stoptimes.txt");
        gtfsData.toHluwStoptimes(out_stoptimes);

        // note : this conversion is only necessary so that Graph doesn't depend on GtfsParsing :
        std::cout << "Converting stops for walking-graph" << std::endl;
        for (auto& stop : gtfsData.rankedStops) {
            stops.emplace_back(stop.longitude, stop.latitude, stop.id, stop.name);
        }
    }

    // walking-graph :
    std::cout << "Building walking-graph" << std::endl;
    my::preprocess::WalkingGraph graph{osmFile, polygonFile, stops, walkspeedKmPerHour};

    std::cout << "Dumping WalkingGraph for HL-UW" << std::endl;
    std::ofstream out_graph(outputDir + "walking_graph.json");
    graph.toHluwStructures(hluwOutputDir);

    std::cout << "Dumping WalkingGraph geojson" << std::endl;
    graph.toStream(out_graph);

    std::cout << "All is OK" << std::endl;

    return 0;
}
