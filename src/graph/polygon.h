#pragma once

#include <vector>

#include <boost/geometry.hpp>

namespace uwpreprocess {

using BgDegree = boost::geometry::cs::spherical_equatorial<boost::geometry::degree>;
using BgPoint = boost::geometry::model::point<double, 2, BgDegree>;
using BgPolygon = boost::geometry::model::polygon<BgPoint, false /* CounterClockwise */>;

BgPolygon create_polygon(std::vector<std::pair<double, double>> const& points);
bool is_inside(BgPolygon const& polygon, double lon, double lat);
bool is_empty(BgPolygon const& polygon);

}  // namespace uwpreprocess
