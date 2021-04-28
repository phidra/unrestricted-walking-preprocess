#include <numeric>

#include "ad/cppgtfs/Parser.h"

#include "gtfs_parsed_data.h"

using namespace std;

namespace uwpreprocess {

// note : the dependency to cppgtfs is isolated in static functions in cpp (instead of exposed in headers) :

static RouteLabel _trip_to_route_label(ad::cppgtfs::gtfs::Trip const& trip) {
    RouteLabel to_return;
    // build the label of the trip's route (scientific route, see below).
    // A route label is just the concatenation of its stop's ids :
    //     32+33+34+122+123+125+126
    // precondition : no stopID constains the delimiter '+'

    if (trip.getStopTimes().size() < 2) {
        ostringstream oss;
        oss << "ERROR : route is too small (" << trip.getStopTimes().size() << ") of trip : " << trip.getId();
        throw runtime_error(oss.str());
    }

    string route_id{};

#ifndef NDEBUG
    int previous_departure_time = -1;
#endif

    // precondition : getStopTimes return stops in order
    for (auto const& stoptime : trip.getStopTimes()) {
        auto stop = *(stoptime.getStop());
        route_id.append(stop.getId());
        route_id.append("+");

#ifndef NDEBUG
        // verifying that stop times are properly ordered :
        int current_departure_time = stoptime.getDepartureTime().seconds();
        if (current_departure_time <= previous_departure_time) {
            throw runtime_error("ERROR : stoptimes are not properly ordered !");
        }
        previous_departure_time = current_departure_time;
#endif
    }

    // remove final '+' :
    to_return.label = route_id.substr(0, route_id.size() - 1);
    return to_return;
}

void _add_trip_to_route(ParsedRoute& route, OrderableTripId const& trip_id, ad::cppgtfs::gtfs::Trip const& trip) {
    auto& this_trip_events = route.trips[trip_id];
    for (auto const& stoptime : trip.getStopTimes()) {
        int arrival_time = stoptime.getArrivalTime().seconds();
        int departure_time = stoptime.getDepartureTime().seconds();
        this_trip_events.emplace_back(arrival_time, departure_time);
    }
}

static ad::cppgtfs::gtfs::Stop const& _get_stop(ad::cppgtfs::gtfs::Feed const& feed, string const& stopid) {
    auto stop_ptr = feed.getStops().get(stopid);
    if (stop_ptr == 0) {
        ostringstream oss;
        oss << "ERROR : unable to get stop with id '" << stopid << "' (stop_ptr is 0)";
        throw runtime_error(oss.str());
    }

    return *stop_ptr;
}

static map<RouteLabel, ParsedRoute> _partition_trips_in_routes(ad::cppgtfs::gtfs::Feed const& feed) {
    // This function partitions the trips of the GTFS feed, according to their stops.
    // All The trips with exactly the same set of stops are grouped into a (scientific) 'route'.
    // Once partitionned, a (scientific) route is identified by its RouteLabel.
    // Two trips will have the same route label IF they have excatly the same sequence of stops.

    map<RouteLabel, ParsedRoute> parsed_routes;

    for (auto const& [trip_id, trip_ptr] : feed.getTrips()) {
        auto& trip = *(trip_ptr);

        // in the set, all the trips of a given route are ordered by their departure times
        // precondition = each trip has at least a stop
        int trip_departure_time_seconds = trip.getStopTimes().begin()->getDepartureTime().seconds();

        RouteLabel route_label = _trip_to_route_label(trip);
        ParsedRoute& parsed_route = parsed_routes[route_label];
        _add_trip_to_route(parsed_route, {trip_departure_time_seconds, trip_id}, trip);
    }

    return parsed_routes;
}

[[maybe_unused]] static bool _check_route_partition_consistency(ad::cppgtfs::gtfs::Feed const& feed,
                                                             map<RouteLabel, ParsedRoute> const& partition) {
    // checks that the agregation of the trips of all routes have the same number of trips than feed
    auto nb_trips_in_feed = feed.getTrips().size();
    size_t nb_trips_in_partitions = accumulate(partition.cbegin(), partition.cend(), 0, [](size_t acc, auto const& route_pair) {
        auto const& route = route_pair.second;
        return acc + route.trips.size();
    });
    return nb_trips_in_feed == nb_trips_in_partitions;
}

static pair<vector<RouteLabel>, unordered_map<RouteLabel, size_t>> _rank_routes(
    map<RouteLabel, ParsedRoute> const& routes) {
    // this function ranks the partitioned routes
    // i.e. each route has an arbitrary rank from 0 to N-1 (where N is the number of routes)
    // (this rank will be used to store the routes in a vector)

    size_t routeRank = 0;
    vector<RouteLabel> ranked_routes;
    unordered_map<RouteLabel, size_t> route_to_rank;

    for (auto& [route_label, _] : routes) {
        ranked_routes.push_back(route_label);
        route_to_rank.insert({route_label, routeRank++});
    }

    // Here :
    //   - ranked_routes associates a rank to a route
    //   - route_to_rank allows to retrieve the rank of a given route
    return {move(ranked_routes), move(route_to_rank)};
}

static pair<vector<ParsedStop>, unordered_map<string, size_t>> _rank_stops(map<RouteLabel, ParsedRoute> const& routes,
                                                                          ad::cppgtfs::gtfs::Feed const& feed) {
    // this function ranks the stops (and filter them : stops not used in at least a route are ignored)
    // i.e. each stop has an arbitrary rank from 0 to N-1 (where N is the number of stops)
    // (this rank will be used to store the stops in a vector)

    // first, identify the stops that are used by at least one route :
    set<string> useful_stop_ids;
    for (auto& [route_label, _] : routes) {
        vector<string> stops_of_this_route = route_label.to_stop_ids();
        useful_stop_ids.insert(stops_of_this_route.begin(), stops_of_this_route.end());
    }

    // then, rank them :
    size_t rank = 0;
    vector<ParsedStop> ranked_stops;
    unordered_map<string, size_t> stopid_to_rank;
    for (auto& stopid : useful_stop_ids) {
        Stop const& stop = _get_stop(feed, stopid);
        ranked_stops.emplace_back(stopid, stop.getName(), stop.getLat(), stop.getLng());
        stopid_to_rank.insert({stopid, rank});
        ++rank;
    }

    // Here :
    //   - ranked_stops associates a rank to a stop
    //   - stopid_to_rank allows to retrieve the rank of a given stop
    return {move(ranked_stops), move(stopid_to_rank)};
}

GtfsParsedData::GtfsParsedData(string const& gtfs_folder) {
    ad::cppgtfs::Parser parser;
    ad::cppgtfs::gtfs::Feed feed;
    parser.parse(&feed, gtfs_folder);

    routes = _partition_trips_in_routes(feed);

#ifndef NDEBUG
    bool is_partition_consistent = _check_route_partition_consistency(feed, routes);
    if (!is_partition_consistent) {
        ostringstream oss;
        oss << "ERROR : number of trips after partitioning by route is not the same than number of trips in feed (="
            << feed.getTrips().size() << ")";
        throw runtime_error(oss.str());
    }
#endif

    tie(ranked_routes, route_to_rank) = _rank_routes(routes);
    tie(ranked_stops, stopid_to_rank) = _rank_stops(routes, feed);
}

void GtfsParsedData::to_hluw_stoptimes(std::ostream& out) const {
    // this functions dumps the stoptimes to use in HL-UW
    // FIXME : this should be in HL-UW repo (but for now, it is easier here)

    // these fields are the only ones that are relevant :
    out << "trip_id,arrival_time,departure_time,stop_id,stop_sequence\n";

    for (auto& [route_label, parsed_route] : routes) {
        // the stops of this route :
        vector<string> const& stop_ids = route_label.to_stop_ids();

        // the events of each trips :
        for (auto& [orderable_trip_id, events] : parsed_route.trips) {
            assert(stop_ids.size() == events.size());
            auto const& trip_id = orderable_trip_id.second;
            size_t stop_sequence = 1;  // in GTFS, stop sequence seem to begin at 1
            for (auto& [arrival_time, departure_time] : events) {
                // FIXME : this assumes that trip AND stop ids don't need escaping
                out << trip_id << "," << arrival_time << "," << departure_time << "," << stop_ids[stop_sequence - 1]
                    << "," << stop_sequence << "\n";
                ++stop_sequence;
            }
        }
    }
}

}  // namespace uwpreprocess
