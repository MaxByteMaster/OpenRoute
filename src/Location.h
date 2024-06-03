#ifndef LOCATION_H
#define LOCATION_H

#include "Projection.h"

namespace location {

//! Writing location of device into epsg_4326_point, in case of error returns false
bool RequestDeviceLocation(projection::Epsg4326Point& epsg_4326_point);

} // namespace location

#endif // LOCATION_H
