#include "Projection.h"

#include <iostream>
#include <cmath>

#include <proj.h>

namespace projection {

bool Transform(const Epsg4326Point& epsg_4326_point, Epsg3857Point& epsg_3857_point)
{
    const auto projection_params = proj_create_crs_to_crs(nullptr, "EPSG:4326", "EPSG:3857", nullptr);

    const auto input_coords = proj_coord(epsg_4326_point.longitude, epsg_4326_point.latitude, 0, 0);

    const auto output_coords = proj_trans(projection_params, PJ_DIRECTION::PJ_FWD, input_coords);

    proj_destroy(projection_params);

    if (std::isinf(output_coords.xy.x) || std::isinf(output_coords.xy.y))
    {
        std::cerr << "Projection::Transform Error when projecting longitude: "
                  << epsg_4326_point.longitude << " latitude: "
                  << epsg_4326_point.latitude << std::endl;

        return false;
    }

    epsg_3857_point.x = output_coords.xy.x;
    epsg_3857_point.y = output_coords.xy.y;

    return true;
}

} // namespace projection
