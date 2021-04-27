#pragma once

#include <ostream>
#include <istream>

#include "gtfs_parsed_data.h"

namespace my::preprocess {

void toStream(std::ostream& out, GtfsParsedData const&);
GtfsParsedData fromStream(std::istream& in);

bool _checkSerializationIdempotent(GtfsParsedData const&);

}  // namespace my::preprocess
