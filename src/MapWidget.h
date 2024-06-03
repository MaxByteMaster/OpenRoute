#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <unordered_set>

#include <QWidget>
#include <QGraphicsEllipseItem>

#include "RendererProcessesManager.h"
#include "NavigationManager.h"
#include "MapGraphicsView.h"
#include "MapControlsWidget.h"
#include "Map.h"

class MapWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MapWidget(QWidget* parent = nullptr);

public slots:
    void OnZoomInWheel(const QPointF& zoom_position);
    void OnZoomOutWheel(const QPointF& zoom_position);

    void OnZoomInButtton();
    void OnZoomOutButtton();

    void OnLocationButtonPressed();

    void OnMapClicked(const QPointF& position);

    void OnImageRendered(std::shared_ptr<QPixmap> pixmap, const map::Tile& tile, const unsigned int zoom);

private:
    struct RoadPoint
    {
        RoadPoint()
        {

        }

        RoadPoint(projection::Epsg3857PointUPtr epsg_3857_point_u_ptr, QGraphicsEllipseItem* scene_item)
            : epsg_3857_point_u_ptr(std::move(epsg_3857_point_u_ptr)), scene_item(scene_item), is_set(true)
        {

        }

        projection::Epsg3857PointUPtr epsg_3857_point_u_ptr;
        QGraphicsEllipseItem* scene_item;
        bool is_set = false;
    };

    struct Route
    {
        Route()
        {

        }

        RoadPoint start_point;
        RoadPoint end_point;

        std::deque<QGraphicsLineItem*> line_deque;
    };

    struct RelativeScenePoint
    {
        RelativeScenePoint(double x, double y)
            : x(x), y(y)
        {

        }

        double x;
        double y;
    };

    RendererProcessesManagerUPtr renderer_processes_manager_u_ptr_;
    NavigationManagerUPtr navigation_manager_u_ptr_;

    QGraphicsScene scene_;
    MapGraphicsView graphics_view_;

    unsigned int zoom_ = 0;
    int axis_tile_count_;
    int max_axis_index_;
    double tile_epsg_3857_length_;
    double pixel_epsg_3857_length_;

    const double kSceneLowerBoundPixel = 0;
    double scene_upper_bound_pixel_;

    std::unordered_set<std::string> visible_tiles_u_set_;

    MapControlsWidget map_controls_widget_;

    Route route_;

    void InitConnections() const;
    void InitLayout();
    void InitMapControls();
    void InitializeMapProperties();

    void UpdateMapProperties();

    void UpdateMapWithNewZoom(const RelativeScenePoint&& relative_zoom_position);
    void UpdateMapCenter(const RelativeScenePoint& relative_scene_point);
    void UpdateMap();

    void RenderTileRect(const map::TileRect& tile_rect);
    void RenderTile(const unsigned int x_index, const unsigned int y_index);

    void Zoom(const QPointF& zoom_position);

    void DrawRoute();
};

#endif // MAPWIDGET_H
