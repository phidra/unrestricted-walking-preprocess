#include <numeric>

#include "ad/cppgtfs/Parser.h"

#include "gtfs_parsed_data.h"

using namespace std;

namespace my::preprocess {

// note : the dependency to cppgtfs is isolated in static functions in cpp (instead of exposed in headers) :

static RouteLabel _tripToRouteLabel(ad::cppgtfs::gtfs::Trip const& trip) {
    RouteLabel toReturn;
    // build the label of the trip's route (scientific route, see below).
    // A route label is just the concatenation of its stop's ids :
    //     32+33+34+122+123+125+126
    // precondition : no stopID constains the delimiter '+'

    if (trip.getStopTimes().size() < 2) {
        ostringstream oss;
        oss << "ERROR : route is too small (" << trip.getStopTimes().size() << ") of trip : " << trip.getId();
        throw runtime_error(oss.str());
    }

    string routeId{};

#ifndef NDEBUG
    int previousDepartureTime = -1;
#endif

    // precondition : getStopTimes return stops in order
    for (auto const& stoptime : trip.getStopTimes()) {
        auto stop = *(stoptime.getStop());
        routeId.append(stop.getId());
        routeId.append("+");

#ifndef NDEBUG
        // verifying that stop times are properly ordered :
        int currentDepartureTime = stoptime.getDepartureTime().seconds();
        if (currentDepartureTime <= previousDepartureTime) {
            throw runtime_error("ERROR : stoptimes are not properly ordered !");
        }
        previousDepartureTime = currentDepartureTime;
#endif
    }

    // remove final '+' :
    toReturn.label = routeId.substr(0, routeId.size() - 1);
    return toReturn;
}

void _addTripToRoute(ParsedRoute& route, OrderableTripId const& tripId, ad::cppgtfs::gtfs::Trip const& trip) {
    auto& thisTripEvents = route.trips[tripId];
    for (auto const& stoptime : trip.getStopTimes()) {
        int arrivalTime = stoptime.getArrivalTime().seconds();
        int departureTime = stoptime.getDepartureTime().seconds();
        thisTripEvents.emplace_back(arrivalTime, departureTime);
    }
}

static ad::cppgtfs::gtfs::Stop const& _getStop(ad::cppgtfs::gtfs::Feed const& feed, string const& stopid) {
    auto stopPtr = feed.getStops().get(stopid);
    if (stopPtr == 0) {
        ostringstream oss;
        oss << "ERROR : unable to get stop with id '" << stopid << "' (stopPtr is 0)";
        throw runtime_error(oss.str());
    }

    return *stopPtr;
}

static map<RouteLabel, ParsedRoute> _partitionTripsInRoutes(ad::cppgtfs::gtfs::Feed const& feed) {
    // This function partitions the trips of the GTFS feed, according to their stops.
    // All The trips with exactly the same set of stops are grouped into a (scientific) 'route'.
    // Once partitionned, a (scientific) route is identified by its RouteLabel.
    // Two trips will have the same route label IF they have excatly the same sequence of stops.

    map<RouteLabel, ParsedRoute> parsedRoutes;

    for (auto const& [tripId, tripPtr] : feed.getTrips()) {
        auto& trip = *(tripPtr);

        // in the set, all the trips of a given route are ordered by their departure times
        // precondition = each trip has at least a stop
        int tripDepartureTimeSeconds = trip.getStopTimes().begin()->getDepartureTime().seconds();

        RouteLabel routeLabel = _tripToRouteLabel(trip);
        ParsedRoute& parsedRoute = parsedRoutes[routeLabel];
        _addTripToRoute(parsedRoute, {tripDepartureTimeSeconds, tripId}, trip);
    }

    return parsedRoutes;
}

[[maybe_unused]] static bool _checkRoutePartitionConsistency(ad::cppgtfs::gtfs::Feed const& feed,
                                                             map<RouteLabel, ParsedRoute> const& partition) {
    // checks that the agregation of the trips of all routes have the same number of trips than feed
    auto nbTripsInFeed = feed.getTrips().size();
    int nbTripsInPartitions = accumulate(partition.cbegin(), partition.cend(), 0, [](int acc, auto const& routePair) {
        auto const& route = routePair.second;
        return acc + route.trips.size();
    });
    return nbTripsInFeed == nbTripsInPartitions;
}

static pair<vector<RouteLabel>, unordered_map<RouteLabel, size_t>> _rankRoutes(
    map<RouteLabel, ParsedRoute> const& routes) {
    // this function ranks the partitioned routes
    // i.e. each route has an arbitrary rank from 0 to N-1 (where N is the number of routes)
    // (this rank will be used to store the routes in a vector)

    size_t routeRank = 0;
    vector<RouteLabel> rankedRoutes;
    unordered_map<RouteLabel, size_t> routeToRank;

    for (auto& [routeLabel, _] : routes) {
        rankedRoutes.push_back(routeLabel);
        routeToRank.insert({routeLabel, routeRank++});
    }

    // Here :
    //   - rankedRoutes associates a rank to a route
    //   - routeToRank allows to retrieve the rank of a given route
    return {move(rankedRoutes), move(routeToRank)};
}

static pair<vector<ParsedStop>, unordered_map<string, size_t>> _rankStops(map<RouteLabel, ParsedRoute> const& routes,
                                                                          ad::cppgtfs::gtfs::Feed const& feed) {
    // this function ranks the stops (and filter them : stops not used in at least a route are ignored)
    // i.e. each stop has an arbitrary rank from 0 to N-1 (where N is the number of stops)
    // (this rank will be used to store the stops in a vector)

    // first, identify the stops that are used by at least one route :
    set<string> usefulStopIds;
    for (auto& [routeLabel, _] : routes) {
        vector<string> stopsOfThisRoute = routeLabel.toStopIds();
        usefulStopIds.insert(stopsOfThisRoute.begin(), stopsOfThisRoute.end());
    }

    // then, rank them :
    size_t rank = 0;
    vector<ParsedStop> rankedStops;
    unordered_map<string, size_t> stopidToRank;
    for (auto& stopid : usefulStopIds) {
        Stop const& stop = _getStop(feed, stopid);
        rankedStops.emplace_back(stopid, stop.getName(), stop.getLat(), stop.getLng());
        stopidToRank.insert({stopid, rank});
        ++rank;
    }

    // Here :
    //   - rankedStops associates a rank to a stop
    //   - stopidToRank allows to retrieve the rank of a given stop
    return {move(rankedStops), move(stopidToRank)};
}

GtfsParsedData::GtfsParsedData(string const& gtfsFolder) {
    ad::cppgtfs::Parser parser;
    ad::cppgtfs::gtfs::Feed feed;
    parser.parse(&feed, gtfsFolder);

    routes = _partitionTripsInRoutes(feed);

#ifndef NDEBUG
    bool isPartitionConsistent = _checkRoutePartitionConsistency(feed, routes);
    if (!isPartitionConsistent) {
        ostringstream oss;
        oss << "ERROR : number of trips after partitioning by route is not the same than number of trips in feed (="
            << feed.getTrips().size() << ")";
        throw runtime_error(oss.str());
    }
#endif

    tie(rankedRoutes, routeToRank) = _rankRoutes(routes);
    tie(rankedStops, stopidToRank) = _rankStops(routes, feed);
}

void GtfsParsedData::toHluwStoptimes(std::ostream& out) const {
    // this functions dumps the stoptimes to use in HL-UW
    // FIXME : this should be in HL-UW repo (but for now, it is easier here)

    // these fields are the only ones that are relevant :
    out << "trip_id,arrival_time,departure_time,stop_id,stop_sequence\n";

    for (auto& [routeLabel, parsedRoute] : routes) {
        // the stops of this route :
        vector<string> const& stop_ids = routeLabel.toStopIds();

        // the events of each trips :
        for (auto& [orderableTripId, events] : parsedRoute.trips) {
            assert(stop_ids.size() == events.size());
            auto const& trip_id = orderableTripId.second;
            size_t stop_sequence = 1;  // in GTFS, stop sequence seem to begin at 1
            for (auto& [arrivalTime, departureTime] : events) {
                // FIXME : this assumes that trip AND stop ids don't need escaping
                out << trip_id << "," << arrivalTime << "," << departureTime << "," << stop_ids[stop_sequence - 1]
                    << "," << stop_sequence << "\n";
                ++stop_sequence;
            }
        }
    }
}

}  // namespace my::preprocess
