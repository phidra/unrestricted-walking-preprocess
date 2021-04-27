#include "gtfs_geojson.h"

#include <fstream>
#include <sstream>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/istreamwrapper.h>

using namespace std;

namespace my::preprocess {

void toStream(ostream& out, GtfsParsedData const& gtfsData) {
    rapidjson::Document doc(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& a = doc.GetAllocator();

    // rankedRoutes
    rapidjson::Value rankedRoutesJson(rapidjson::kArrayType);
    for (auto& route : gtfsData.rankedRoutes) {
        rankedRoutesJson.PushBack(rapidjson::Value().SetString(route.label.c_str(), a), a);
    }
    doc.AddMember("rankedRoutes", rankedRoutesJson, a);

    // rankedStops
    rapidjson::Value rankedStopsJson(rapidjson::kArrayType);
    for (auto& parsedStop : gtfsData.rankedStops) {
        rapidjson::Value stop(rapidjson::kObjectType);
        stop.AddMember("latitude", parsedStop.latitude, a);
        stop.AddMember("longitude", parsedStop.longitude, a);
        stop.AddMember("id", rapidjson::Value().SetString(parsedStop.id.c_str(), a), a);
        stop.AddMember("name", rapidjson::Value().SetString(parsedStop.name.c_str(), a), a);
        rankedStopsJson.PushBack(stop, a);
    }
    doc.AddMember("rankedStops", rankedStopsJson, a);

    // routes
    // routes are stored in a map that associates a label (key) to trips (value)
    // trips are themselves a map that associates an OrderableTripId to a vector of events
    //
    // Détaillons un chouïa la map des trips :
    // Comme la map est ordonnée, il faut que je stocke une liste de pair{KEY|VALUE}.
    //     KEY   = pair{TripEventTime=int|string}
    //     VALUE = vector<Event>   avvec Event=pair{int|int}
    rapidjson::Value routesJson(rapidjson::kArrayType);
    for (auto& [routeLabel, route] : gtfsData.routes) {
        rapidjson::Value tripsJson(rapidjson::kArrayType);
        for (auto& [tripid, tripEvents] : route.trips) {
            // map-key = OrderableTripId = pair<TripEventTime, string>  (avec TripEventTime=int)
            rapidjson::Value orderableTripIdJson(rapidjson::kArrayType);
            auto& [tripEventTime, id] = tripid;
            orderableTripIdJson.PushBack(tripEventTime, a);
            orderableTripIdJson.PushBack(rapidjson::Value().SetString(id.c_str(), a), a);

            // map-value = vector d'events = vector de pair<int, int> :
            rapidjson::Value eventsJson(rapidjson::kArrayType);
            for (auto& [departure, arrival] : tripEvents) {
                rapidjson::Value eventPairJson(rapidjson::kArrayType);
                eventPairJson.PushBack(departure, a);
                eventPairJson.PushBack(arrival, a);
                eventsJson.PushBack(eventPairJson, a);
            }

            // on stocke chaque élément de la map comme une pair{key, value} (afin de conserver l'ordre de la map)
            rapidjson::Value mapPairJson(rapidjson::kArrayType);
            mapPairJson.PushBack(orderableTripIdJson, a);
            mapPairJson.PushBack(eventsJson, a);
            tripsJson.PushBack(mapPairJson, a);
        }

        // rebelote : on stocke chaque élément de la map comme une pair{key, value} :
        rapidjson::Value mapPairJson(rapidjson::kArrayType);
        mapPairJson.PushBack(rapidjson::Value().SetString(routeLabel.label.c_str(), a), a);
        mapPairJson.PushBack(tripsJson, a);
        routesJson.PushBack(mapPairJson, a);
    }
    doc.AddMember("routes", routesJson, a);

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

GtfsParsedData fromStream(istream& in) {
    rapidjson::IStreamWrapper stream_wrapper(in);
    rapidjson::Document doc;
    doc.ParseStream(stream_wrapper);

    assert_geojson_format(doc.IsObject(), "doc is not an object");
    assert_geojson_format(doc.HasMember("rankedRoutes"), "doc has no 'rankedRoutes'");

    // DESERIALIZATION rankedRoutes :
    auto& rankedRoutesJson = doc["rankedRoutes"];
    assert_geojson_format(rankedRoutesJson.IsArray(), "rankedRoutes is not an array");
    vector<RouteLabel> rankedRoutes;
    size_t routeRank = 0;
    unordered_map<RouteLabel, size_t> routeToRank;
    for (auto ite = rankedRoutesJson.Begin(); ite != rankedRoutesJson.End(); ++ite, ++routeRank) {
        assert_geojson_format(ite->IsString(), "label is not a string");
        RouteLabel label{ite->GetString()};
        rankedRoutes.emplace_back(label);
        routeToRank[label] = routeRank;
    }

    // DESERIALIZATION rankedStops :
    auto& rankedStopsJson = doc["rankedStops"];
    assert_geojson_format(rankedStopsJson.IsArray(), "rankedStops is not an array");
    vector<ParsedStop> rankedStops;
    size_t stopRank = 0;
    unordered_map<string, size_t> stopidToRank;
    for (auto ite = rankedStopsJson.Begin(); ite != rankedStopsJson.End(); ++ite, ++stopRank) {
        auto& stopJson = *ite;
        assert_geojson_format(stopJson.IsObject(), "stop is not an object");
        assert_geojson_format(stopJson.HasMember("latitude"), "stop has no 'latitude'");
        assert_geojson_format(stopJson.HasMember("longitude"), "stop has no 'longitude'");
        assert_geojson_format(stopJson.HasMember("id"), "stop has no 'id'");
        assert_geojson_format(stopJson.HasMember("name"), "stop has no 'name'");
        assert_geojson_format(stopJson["latitude"].IsDouble(), "latitude is not a double");
        assert_geojson_format(stopJson["longitude"].IsDouble(), "longitude is not a double");
        assert_geojson_format(stopJson["id"].IsString(), "id is not a string");
        assert_geojson_format(stopJson["name"].IsString(), "name is not a string");

        ParsedStop stop{stopJson["id"].GetString(), stopJson["name"].GetString(), stopJson["latitude"].GetDouble(),
                        stopJson["longitude"].GetDouble()};
        rankedStops.push_back(stop);
        stopidToRank[stop.id] = stopRank;
    }

    // DESERIALIZATION routes :
    auto& routesJson = doc["routes"];
    assert_geojson_format(routesJson.IsArray(), "routes is not an array");
    map<RouteLabel, ParsedRoute> routes;
    for (auto ite = routesJson.Begin(); ite != routesJson.End(); ++ite) {
        assert_geojson_format(ite->IsArray(), "routepair-iterator is not an array");
        assert_geojson_format(ite->Size() == 2, "routepair should have 2 elements");

        // left-element of the route-pair = key of the map = RouteLabel :
        auto& labelJson = (*ite)[0];
        assert_geojson_format(labelJson.IsString(), "label is not a string");
        RouteLabel label{labelJson.GetString()};

        // right-element of the route-pair = value of the map = the trips of the ParsedRoute :
        auto& tripsJson = (*ite)[1];
        assert_geojson_format(tripsJson.IsArray(), "trips is not an array");

        ParsedRoute::Trips trips;
        for (auto itebis = tripsJson.Begin(); itebis != tripsJson.End(); ++itebis) {
            assert_geojson_format(itebis->IsArray(), "trippair-iterator is not an array");
            assert_geojson_format(itebis->Size() == 2, "trippair should have 2 elements");

            // left-element of the trip-pair = key of the submap = OrderableTripId :
            auto& orderableTripIdJson = (*itebis)[0];
            assert_geojson_format(orderableTripIdJson.IsArray(), "orderabletripid is not an array");
            assert_geojson_format(orderableTripIdJson.Size() == 2, "orderabletripid should have 2 elements");

            auto& tripEventTimeJson = orderableTripIdJson[0];
            assert_geojson_format(tripEventTimeJson.IsInt(), "tripEventTime should be an int");
            int tripEventTime = tripEventTimeJson.GetInt();

            auto& tripIdJson = orderableTripIdJson[1];
            assert_geojson_format(tripIdJson.IsString(), "tripId should be a string");
            string tripId = tripIdJson.GetString();

            OrderableTripId otid{tripEventTime, tripId};

            // right-element of the trip-pair = value of the submap = the vector of stopEvents
            auto& stopEventsJson = (*itebis)[1];
            assert_geojson_format(stopEventsJson.IsArray(), "stopevents is not an array");
            vector<ParsedRoute::StopEvent> stopEvents;
            for (auto iteter = stopEventsJson.Begin(); iteter != stopEventsJson.End(); ++iteter) {
                assert_geojson_format(iteter->IsArray(), "eventpair-iterator is not an array");
                assert_geojson_format(iteter->Size() == 2, "eventpair should have 2 elements");
                assert_geojson_format((*iteter)[0].IsInt(), "event-left should be an int");
                assert_geojson_format((*iteter)[1].IsInt(), "event-right should be an int");
                int arrival = (*iteter)[0].GetInt();
                int departure = (*iteter)[1].GetInt();
                stopEvents.emplace_back(arrival, departure);
            }
            trips.insert({otid, stopEvents});
        }

        ParsedRoute pr(move(trips));
        routes.insert({label, pr});
    }

    GtfsParsedData toReturn;
    toReturn.rankedRoutes = rankedRoutes;
    toReturn.routeToRank = routeToRank;
    toReturn.rankedStops = rankedStops;
    toReturn.stopidToRank = stopidToRank;
    toReturn.routes = routes;

    return toReturn;
}

bool _checkSerializationIdempotent(GtfsParsedData const& gtfs) {
    // serializing in a temporary file :
    ostringstream oss;
    toStream(oss, gtfs);

    istringstream iss(oss.str());
    GtfsParsedData deserialized = fromStream(iss);
    return deserialized == gtfs;
}

}  // namespace my::preprocess
