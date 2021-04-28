#pragma once

#include <ostream>
#include <istream>

#include "gtfs_parsed_data.h"

namespace uwpreprocess {

void to_stream(std::ostream& out, GtfsParsedData const&);
GtfsParsedData from_stream(std::istream& in);

bool _check_serialization_idempotent(GtfsParsedData const&);

}  // namespace uwpreprocess
