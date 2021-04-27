#pragma once

#include <vector>
#include <functional>

#include "gtfs_parsing_structures.h"

// From a given GTFS feed, GtfsParsedData is an abstraction of the GTFS data, suitable for ULTRA :
//  - only the stops that appear in at least one trip are kept (unused stops are ignored)
//  - trips are partitionned into "scientific" routes (about routes, see details below)
//  - routes and stops are ranked (about ranks, see details below)
//  - a route (or a stop) can be identified with either its "ID" (RouteLabel/StopId) or its rank
//  - the conversion between ID<->rank is done with the conversion structures

// NOTE : the code that builds GtfsParsedData is currently tightly coupled to cppgtfs, the gtfs-parsing library

// WARNING : there are two mismatching definitions of the word "route" :
//  - what scientific papers calls "route" is a particular set of stops
//    in particular, if two trips travel between exactly the same stops, they belong to the same route.
//  - what GTFS standard (and thus cppgtfs) calls "route" is just a given structure associated to a trip
//    but this association is arbitrary : in GTFS data, two trips can use the same "route" structure
//    even if they don't use exactly the same set of stops
//
// In general, in ULTRA code (and in code building ULTRA data), the "routes" are the scientific ones.
// Thus, one of the main purpose of GtfsParsedData is to build "scientific" routes from GTFS feed.
// BEWARE : the "routes" returned by cppgtfs are not the scientific ones, and they are NOT further used !

namespace my::preprocess {

struct GtfsParsedData {
    GtfsParsedData(std::string const& gtfsFolder);
    inline GtfsParsedData(){};

    std::map<RouteLabel, ParsedRoute> routes;

    // stops and routes are ranked
    // each stop/route has an arbitrary rank from 0 to N-1 (where N is the number of stops/routes)
    // this rank will be used to store the stops/routes in a vector
    // the following conversion structures allow to convert between rank and label/id :

    //   - rankedRoutes associates a rank to a route
    //   - routeToRank allows to retrieve the rank of a given route
    std::vector<RouteLabel> rankedRoutes;
    std::unordered_map<RouteLabel, size_t> routeToRank;

    //   - rankedStops associates a rank to a stop
    //   - stopidToRank allows to retrieve the rank of a given stop
    std::vector<ParsedStop> rankedStops;
    std::unordered_map<std::string, size_t> stopidToRank;

    // serialization/deserialization :
    void toHluwStoptimes(std::ostream& out) const;  // FIXME : this should be in HL-UW repo

    inline bool operator==(GtfsParsedData const& other) const {
        return (rankedRoutes == other.rankedRoutes && routeToRank == other.routeToRank &&
                rankedStops == other.rankedStops && stopidToRank == other.stopidToRank && routes == other.routes);
    }
};

}  // namespace my::preprocess
