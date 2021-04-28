#include "gtfs_geojson.h"

#include <fstream>
#include <sstream>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/istreamwrapper.h>

using namespace std;

namespace uwpreprocess {

void to_stream(ostream& out, GtfsParsedData const& gtfs_data) {
    rapidjson::Document doc(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& a = doc.GetAllocator();

    // ranked_routes
    rapidjson::Value ranked_routes_json(rapidjson::kArrayType);
    for (auto& route : gtfs_data.ranked_routes) {
        ranked_routes_json.PushBack(rapidjson::Value().SetString(route.label.c_str(), a), a);
    }
    doc.AddMember("rankedRoutes", ranked_routes_json, a);

    // ranked_stops
    rapidjson::Value ranked_stops_json(rapidjson::kArrayType);
    for (auto& parsed_stop : gtfs_data.ranked_stops) {
        rapidjson::Value stop(rapidjson::kObjectType);
        stop.AddMember("latitude", parsed_stop.latitude, a);
        stop.AddMember("longitude", parsed_stop.longitude, a);
        stop.AddMember("id", rapidjson::Value().SetString(parsed_stop.id.c_str(), a), a);
        stop.AddMember("name", rapidjson::Value().SetString(parsed_stop.name.c_str(), a), a);
        ranked_stops_json.PushBack(stop, a);
    }
    doc.AddMember("rankedStops", ranked_stops_json, a);

    // routes
    // routes are stored in a map that associates a label (key) to trips (value)
    // trips are themselves a map that associates an OrderableTripId to a vector of events
    //
    // Détaillons un chouïa la map des trips :
    // Comme la map est ordonnée, il faut que je stocke une liste de pair{KEY|VALUE}.
    //     KEY   = pair{TripEventTime=int|string}
    //     VALUE = vector<Event>   avvec Event=pair{int|int}
    rapidjson::Value routes_json(rapidjson::kArrayType);
    for (auto& [route_label, route] : gtfs_data.routes) {
        rapidjson::Value trips_json(rapidjson::kArrayType);
        for (auto& [tripid, trip_events] : route.trips) {
            // map-key = OrderableTripId = pair<TripEventTime, string>  (avec TripEventTime=int)
            rapidjson::Value orderable_trip_id_json(rapidjson::kArrayType);
            auto& [trip_event_time, id] = tripid;
            orderable_trip_id_json.PushBack(trip_event_time, a);
            orderable_trip_id_json.PushBack(rapidjson::Value().SetString(id.c_str(), a), a);

            // map-value = vector d'events = vector de pair<int, int> :
            rapidjson::Value events_json(rapidjson::kArrayType);
            for (auto& [departure, arrival] : trip_events) {
                rapidjson::Value event_pair_json(rapidjson::kArrayType);
                event_pair_json.PushBack(departure, a);
                event_pair_json.PushBack(arrival, a);
                events_json.PushBack(event_pair_json, a);
            }

            // on stocke chaque élément de la map comme une pair{key, value} (afin de conserver l'ordre de la map)
            rapidjson::Value map_pair_json(rapidjson::kArrayType);
            map_pair_json.PushBack(orderable_trip_id_json, a);
            map_pair_json.PushBack(events_json, a);
            trips_json.PushBack(map_pair_json, a);
        }

        // rebelote : on stocke chaque élément de la map comme une pair{key, value} :
        rapidjson::Value map_pair_json(rapidjson::kArrayType);
        map_pair_json.PushBack(rapidjson::Value().SetString(route_label.label.c_str(), a), a);
        map_pair_json.PushBack(trips_json, a);
        routes_json.PushBack(map_pair_json, a);
    }
    doc.AddMember("routes", routes_json, a);

    // dumping :
    rapidjson::OStreamWrapper out_wrapper(out);
    rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(out_wrapper);
    doc.Accept(writer);
}

struct IllFormattedGtfsDataException : public exception {
    IllFormattedGtfsDataException(string description) : msg{string("Ill-formatted gtfs-data file : ") + description} {}
    const char* what() const throw() { return msg.c_str(); }
    string msg;
};

static void assert_geojson_format(bool condition, string description) {
    if (!condition)
        throw IllFormattedGtfsDataException{description};
}

GtfsParsedData from_stream(istream& in) {
    rapidjson::IStreamWrapper stream_wrapper(in);
    rapidjson::Document doc;
    doc.ParseStream(stream_wrapper);

    assert_geojson_format(doc.IsObject(), "doc is not an object");
    assert_geojson_format(doc.HasMember("rankedRoutes"), "doc has no 'ranked_routes'");

    // DESERIALIZATION ranked_routes :
    auto& ranked_routes_json = doc["rankedRoutes"];
    assert_geojson_format(ranked_routes_json.IsArray(), "ranked_routes is not an array");
    vector<RouteLabel> ranked_routes;
    size_t route_rank = 0;
    unordered_map<RouteLabel, size_t> route_to_rank;
    for (auto ite = ranked_routes_json.Begin(); ite != ranked_routes_json.End(); ++ite, ++route_rank) {
        assert_geojson_format(ite->IsString(), "label is not a string");
        RouteLabel label{ite->GetString()};
        ranked_routes.emplace_back(label);
        route_to_rank[label] = route_rank;
    }

    // DESERIALIZATION ranked_stops :
    auto& ranked_stops_json = doc["rankedStops"];
    assert_geojson_format(ranked_stops_json.IsArray(), "ranked_stops is not an array");
    vector<ParsedStop> ranked_stops;
    size_t stop_rank = 0;
    unordered_map<string, size_t> stopid_to_rank;
    for (auto ite = ranked_stops_json.Begin(); ite != ranked_stops_json.End(); ++ite, ++stop_rank) {
        auto& stop_json = *ite;
        assert_geojson_format(stop_json.IsObject(), "stop is not an object");
        assert_geojson_format(stop_json.HasMember("latitude"), "stop has no 'latitude'");
        assert_geojson_format(stop_json.HasMember("longitude"), "stop has no 'longitude'");
        assert_geojson_format(stop_json.HasMember("id"), "stop has no 'id'");
        assert_geojson_format(stop_json.HasMember("name"), "stop has no 'name'");
        assert_geojson_format(stop_json["latitude"].IsDouble(), "latitude is not a double");
        assert_geojson_format(stop_json["longitude"].IsDouble(), "longitude is not a double");
        assert_geojson_format(stop_json["id"].IsString(), "id is not a string");
        assert_geojson_format(stop_json["name"].IsString(), "name is not a string");

        ParsedStop stop{stop_json["id"].GetString(), stop_json["name"].GetString(), stop_json["latitude"].GetDouble(),
                        stop_json["longitude"].GetDouble()};
        ranked_stops.push_back(stop);
        stopid_to_rank[stop.id] = stop_rank;
    }

    // DESERIALIZATION routes :
    auto& routes_json = doc["routes"];
    assert_geojson_format(routes_json.IsArray(), "routes is not an array");
    map<RouteLabel, ParsedRoute> routes;
    for (auto ite = routes_json.Begin(); ite != routes_json.End(); ++ite) {
        assert_geojson_format(ite->IsArray(), "routepair-iterator is not an array");
        assert_geojson_format(ite->Size() == 2, "routepair should have 2 elements");

        // left-element of the route-pair = key of the map = RouteLabel :
        auto& label_json = (*ite)[0];
        assert_geojson_format(label_json.IsString(), "label is not a string");
        RouteLabel label{label_json.GetString()};

        // right-element of the route-pair = value of the map = the trips of the ParsedRoute :
        auto& trips_json = (*ite)[1];
        assert_geojson_format(trips_json.IsArray(), "trips is not an array");

        ParsedRoute::Trips trips;
        for (auto ite_bis = trips_json.Begin(); ite_bis != trips_json.End(); ++ite_bis) {
            assert_geojson_format(ite_bis->IsArray(), "trippair-iterator is not an array");
            assert_geojson_format(ite_bis->Size() == 2, "trippair should have 2 elements");

            // left-element of the trip-pair = key of the submap = OrderableTripId :
            auto& orderable_trip_id_json = (*ite_bis)[0];
            assert_geojson_format(orderable_trip_id_json.IsArray(), "orderabletripid is not an array");
            assert_geojson_format(orderable_trip_id_json.Size() == 2, "orderabletripid should have 2 elements");

            auto& trip_event_time_json = orderable_trip_id_json[0];
            assert_geojson_format(trip_event_time_json.IsInt(), "trip_event_time should be an int");
            int trip_event_time = trip_event_time_json.GetInt();

            auto& trip_id_json = orderable_trip_id_json[1];
            assert_geojson_format(trip_id_json.IsString(), "trip_id should be a string");
            string trip_id = trip_id_json.GetString();

            OrderableTripId otid{trip_event_time, trip_id};

            // right-element of the trip-pair = value of the submap = the vector of stop_events
            auto& stop_events_json = (*ite_bis)[1];
            assert_geojson_format(stop_events_json.IsArray(), "stopevents is not an array");
            vector<ParsedRoute::StopEvent> stop_events;
            for (auto ite_ter = stop_events_json.Begin(); ite_ter != stop_events_json.End(); ++ite_ter) {
                assert_geojson_format(ite_ter->IsArray(), "eventpair-iterator is not an array");
                assert_geojson_format(ite_ter->Size() == 2, "eventpair should have 2 elements");
                assert_geojson_format((*ite_ter)[0].IsInt(), "event-left should be an int");
                assert_geojson_format((*ite_ter)[1].IsInt(), "event-right should be an int");
                int arrival = (*ite_ter)[0].GetInt();
                int departure = (*ite_ter)[1].GetInt();
                stop_events.emplace_back(arrival, departure);
            }
            trips.insert({otid, stop_events});
        }

        ParsedRoute pr(move(trips));
        routes.insert({label, pr});
    }

    GtfsParsedData to_return;
    to_return.ranked_routes = ranked_routes;
    to_return.route_to_rank = route_to_rank;
    to_return.ranked_stops = ranked_stops;
    to_return.stopid_to_rank = stopid_to_rank;
    to_return.routes = routes;

    return to_return;
}

bool _check_serialization_idempotent(GtfsParsedData const& gtfs) {
    // serializing in a temporary file :
    ostringstream oss;
    to_stream(oss, gtfs);

    istringstream iss(oss.str());
    GtfsParsedData deserialized = from_stream(iss);
    return deserialized == gtfs;
}

}  // namespace uwpreprocess
