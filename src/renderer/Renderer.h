#ifndef RENDERER_H
#define RENDERER_H

#include "../Projection.h"

#include <QPixmap>
#include <mapnik/map.hpp>

class Renderer
{
public:
    Renderer();

    void RenderTile(const projection::Epsg3857Rect&& epsg_3857_rect, const unsigned int image_index);

private:
    mapnik::Map map_;
    mapnik::image_rgba8 image_;
};

#endif // RENDERER_H
