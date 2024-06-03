#include "Renderer.h"

#include <mapnik/image.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/load_map.hpp>
#include <mapnik/datasource_cache.hpp>

#include <QApplication>

constexpr int kTileSize { 256 };

Renderer::Renderer()
{
    const auto stylesheet_path = std::string("renderer/openstreetmap-carto/mapnik.xml");

    mapnik::datasource_cache::instance().register_datasources("/usr/lib/mapnik/input/postgis.input");

    map_ = mapnik::Map(kTileSize, kTileSize);
    mapnik::load_map(map_, stylesheet_path);

    image_ = mapnik::image_rgba8 {kTileSize, kTileSize};
}

void Renderer::RenderTile(const projection::Epsg3857Rect&& epsg_3857_rect, const unsigned int image_index)
{
    const auto box = mapnik::box2d<double>(epsg_3857_rect.bottom_left_point.x, epsg_3857_rect.bottom_left_point.y, epsg_3857_rect.top_right_point.x, epsg_3857_rect.top_right_point.y);
    map_.zoom_to_box(box);

    auto renderer = mapnik::agg_renderer<mapnik::image_rgba8>(map_, image_);
    renderer.apply();

    mapnik::save_to_file(image_, QString("renderer/renderer_process%1_output.png").arg(image_index).toStdString(), "png");
}

/* This code intended to run as a separate process managed
by RendererProcessesManager class from main application */
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    const auto process_index = QApplication::arguments()[1].toUInt();

    auto renderer = Renderer();
    double left, bottom, right, top;
    unsigned int x_index, y_index, zoom;

    for (;;)
    {
        std::cin >> left >> bottom >> right >> top;
        std::cin >> x_index >> y_index;
        std::cin >> zoom;

        renderer.RenderTile(projection::Epsg3857Rect(left, bottom, right, top), process_index);

        /* Attention! QProcess::readyReadStandardOutput signal emitted every
        time data written in cout. So to avoid multiple signals after finishing
        rendering use endl only in the end of data output */
        std::cout << x_index << " " << y_index << " " << zoom << std::endl;
    }

    return a.exec();
}
