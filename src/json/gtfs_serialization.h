#pragma once

#include <ostream>
#include <istream>

#include "gtfs/gtfs_parsed_data.h"

namespace uwpreprocess::json {

void serialize_gtfs(GtfsParsedData const&, std::ostream&);
GtfsParsedData unserialize_gtfs(std::istream& in);

bool _check_serialization_idempotent(GtfsParsedData const&);

}  // namespace uwpreprocess
