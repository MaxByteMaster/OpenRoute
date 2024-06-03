#ifndef PROJECTION_H
#define PROJECTION_H

#include <memory>

namespace projection {

//! Web Mercator (meters)
struct Epsg3857Point
{
    Epsg3857Point() : x(0), y(0)
    {

    }

    Epsg3857Point(const double x, const double y)
        : x(x), y(y)
    {

    }

    double x;
    double y;
};

using Epsg3857PointUPtr = std::unique_ptr<Epsg3857Point>;

struct Epsg3857Rect
{
    Epsg3857Rect()
    {

    }

    Epsg3857Rect(const double bottom_left_point_x, const double bottom_left_point_y, const double top_right_point_x, const double top_right_point_y)
        : bottom_left_point(bottom_left_point_x, bottom_left_point_y), top_right_point(top_right_point_x, top_right_point_y)
    {

    }

    Epsg3857Point bottom_left_point;
    Epsg3857Point top_right_point;
};

struct Epsg4326Point
{
    Epsg4326Point() : longitude(0), latitude(0)
    {

    }

    Epsg4326Point(const double longitude, const double latitude) : longitude(longitude), latitude(latitude)
    {

    }

    double longitude;
    double latitude;
};

//! Turns coordinates from EPSG:4326 (longitude/latitude) to EPSG:3857 (meters)
bool Transform(const Epsg4326Point& epsg_4326_point, Epsg3857Point& epsg_3857_point);

} // namespace projection

#endif // PROJECTION_H
