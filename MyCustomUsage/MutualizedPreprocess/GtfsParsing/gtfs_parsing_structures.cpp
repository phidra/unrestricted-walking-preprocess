#include <sstream>
#include <cmath>

#include "gtfs_parsing_structures.h"

using namespace std;

namespace my::preprocess {

vector<string> RouteLabel::toStopIds() const {
    // from a given routeLabel, this functions builds back the list of its stop's ids :
    vector<string> stops;
    string token;
    istringstream iss(string(*this));
    while (getline(iss, token, '+')) {
        stops.push_back(token);
    }
    return stops;
}

// there is a slight rounding error in json serialization/deserialization of coordinates
// to make this error unvisible (and allow that using deserialized data is binary iso), we limit
// the double places after the comma.
// At this level of precision, there are no effects on coordinates precision.
constexpr const double trimming_factor = 1e9;
inline double trim(const double x) {
    return round(x * trimming_factor) / trimming_factor;
}

ParsedStop::ParsedStop(string const& id_, string const& name_, double latitude_, double longitude_)
    : id{id_}, name{name_}, latitude{trim(latitude_)}, longitude{trim(longitude_)} {}

string ParsedStop::as_string() const {
    ostringstream oss;
    oss << "ParsedStop{" << id << ", " << name << ", " << latitude << ", " << longitude << "}";
    return oss.str();
};

}  // namespace my::preprocess
