#include "MapWidget.h"

#include <iostream>

#include <QThread>
#include <QScrollBar>
#include <QHBoxLayout>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>

#include "MapControlsWidget.h"
#include "Location.h"

constexpr int kZoomLowerBound { 0 };
constexpr int kZoomUpperBound { 22 };

constexpr int kTilePixelSize { 256 };

constexpr double kMapBoundEpsg3857 { 20037508 };

constexpr int kPenWidth { 5 };

MapWidget::MapWidget(QWidget* parent)
    : QWidget(parent),
    scene_(this),
    graphics_view_(&scene_, this),
    map_controls_widget_(this)
{
    renderer_processes_manager_u_ptr_ = RendererProcessesManager::Create(QThread::idealThreadCount(), this);
    if (!renderer_processes_manager_u_ptr_)
        throw std::runtime_error("Failed to create RendererProcessesManager");

    navigation_manager_u_ptr_ = NavigationManager::Create();
    if (!navigation_manager_u_ptr_)
        throw std::runtime_error("Failed to create NavigationManager");

    InitConnections();
    InitLayout();
    InitMapControls();
    InitializeMapProperties();
}

void MapWidget::InitConnections() const
{
    connect(renderer_processes_manager_u_ptr_.get(), &RendererProcessesManager::ImageRendered, this, &MapWidget::OnImageRendered);

    connect(&graphics_view_, &MapGraphicsView::ZoomIn, this, &MapWidget::OnZoomInWheel);
    connect(&graphics_view_, &MapGraphicsView::ZoomOut, this, &MapWidget::OnZoomOutWheel);
    connect(&graphics_view_, &MapGraphicsView::MapClicked, this, &MapWidget::OnMapClicked);

    connect(graphics_view_.horizontalScrollBar(), &QScrollBar::valueChanged, this, &MapWidget::UpdateMap);
    connect(graphics_view_.verticalScrollBar(), &QScrollBar::valueChanged, this, &MapWidget::UpdateMap);

    connect(&map_controls_widget_, &MapControlsWidget::ZoomIn, this, &MapWidget::OnZoomInButtton);
    connect(&map_controls_widget_, &MapControlsWidget::ZoomOut, this, &MapWidget::OnZoomOutButtton);
    connect(&map_controls_widget_, &MapControlsWidget::LocationButtonPressed, this, &MapWidget::OnLocationButtonPressed);
}

void MapWidget::InitLayout()
{
    graphics_view_.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    graphics_view_.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto layout = new QHBoxLayout(this);
    layout->addWidget(&graphics_view_);
    setLayout(layout);
}

void MapWidget::InitMapControls()
{
    map_controls_widget_.SetZoomRange(kZoomLowerBound, kZoomUpperBound);
    map_controls_widget_.SetCurrentZoom(zoom_);

    layout()->addWidget(&map_controls_widget_);
}

void MapWidget::InitializeMapProperties()
{
    const auto epsg_3857_rect = projection::Epsg3857Rect(-kMapBoundEpsg3857, -kMapBoundEpsg3857, kMapBoundEpsg3857, kMapBoundEpsg3857);
    renderer_processes_manager_u_ptr_->AddRenderingTask(epsg_3857_rect, map::Tile(0, 0), zoom_);

    UpdateMapProperties();
}

void MapWidget::UpdateMapProperties()
{
    axis_tile_count_ = std::pow(2, zoom_);
    max_axis_index_ = axis_tile_count_ - 1;
    tile_epsg_3857_length_ = 2 * kMapBoundEpsg3857 / axis_tile_count_;
    pixel_epsg_3857_length_ = tile_epsg_3857_length_ / kTilePixelSize;
    scene_upper_bound_pixel_ = axis_tile_count_ * kTilePixelSize;
}

void MapWidget::UpdateMapWithNewZoom(const RelativeScenePoint&& relative_zoom_position)
{
    renderer_processes_manager_u_ptr_->ClearRenderingTasks();
    scene_.clear();
    visible_tiles_u_set_.clear();
    route_ = Route();

    UpdateMapProperties();

    scene_.setSceneRect(QRectF(kSceneLowerBoundPixel, kSceneLowerBoundPixel, scene_upper_bound_pixel_, scene_upper_bound_pixel_));
    UpdateMapCenter(relative_zoom_position);

    UpdateMap();
}

void MapWidget::UpdateMapCenter(const RelativeScenePoint& relative_scene_point)
{
    auto map_center = QPointF(relative_scene_point.x * axis_tile_count_, relative_scene_point.y * axis_tile_count_);

    if (map_center.x() < 0)
        map_center.setX(0);
    else if (map_center.x() > scene_upper_bound_pixel_)
        map_center.setX(scene_upper_bound_pixel_);

    if (map_center.y() < 0)
        map_center.setY(0);
    else if (map_center.y() > scene_upper_bound_pixel_)
        map_center.setY(scene_upper_bound_pixel_);

    disconnect(graphics_view_.horizontalScrollBar(), &QScrollBar::valueChanged, this, &MapWidget::UpdateMap);
    disconnect(graphics_view_.verticalScrollBar(), &QScrollBar::valueChanged, this, &MapWidget::UpdateMap);

    graphics_view_.centerOn(map_center.x(), map_center.y());

    connect(graphics_view_.horizontalScrollBar(), &QScrollBar::valueChanged, this, &MapWidget::UpdateMap);
    connect(graphics_view_.verticalScrollBar(), &QScrollBar::valueChanged, this, &MapWidget::UpdateMap);
}

void MapWidget::UpdateMap()
{
    auto tile_rect = map::TileRect();

    // Converting graphics_view rect into scene coordinates to be able to compute which tiles are visible now
    const auto view_rect_in_scene_coordinates = QRect(
        graphics_view_.mapToScene(0, 0).toPoint(),
        graphics_view_.mapToScene(graphics_view_.width(), graphics_view_.height()).toPoint()
    );

    // If visible area greater than map bounds, than render all map
    if (view_rect_in_scene_coordinates.topLeft().x() <= 0)
        tile_rect.bottom_left_tile.x_index = 0;
    else
        tile_rect.bottom_left_tile.x_index = std::floor(view_rect_in_scene_coordinates.topLeft().x() / kTilePixelSize);

    if (view_rect_in_scene_coordinates.topLeft().y() <= 0)
        tile_rect.top_right_tile.y_index = max_axis_index_;
    else
        tile_rect.top_right_tile.y_index = max_axis_index_ - std::floor(view_rect_in_scene_coordinates.topLeft().y() / kTilePixelSize);

    if (view_rect_in_scene_coordinates.bottomRight().x() >= scene_.width())
        tile_rect.top_right_tile.x_index = max_axis_index_;
    else
        tile_rect.top_right_tile.x_index = std::floor(view_rect_in_scene_coordinates.bottomRight().x() / kTilePixelSize);

    if (view_rect_in_scene_coordinates.bottomRight().y() >= scene_.width())
        tile_rect.bottom_left_tile.y_index = 0;
    else
        tile_rect.bottom_left_tile.y_index = max_axis_index_ - std::floor(view_rect_in_scene_coordinates.bottomRight().y() / kTilePixelSize);

    RenderTileRect(tile_rect);
}

void MapWidget::RenderTileRect(const map::TileRect& tile_rect)
{
    for (auto x_index = tile_rect.bottom_left_tile.x_index; x_index <= tile_rect.top_right_tile.x_index; x_index++)
    {
        for (auto y_index = tile_rect.bottom_left_tile.y_index; y_index <= tile_rect.top_right_tile.y_index; y_index++)
        {
            RenderTile(x_index, y_index);
        }
    }
}

void MapWidget::RenderTile(const unsigned int x_index, const unsigned int y_index)
{
    const auto tile_key = QString("%1_%2").arg(x_index).arg(y_index).toStdString();

    if (visible_tiles_u_set_.find(tile_key) != visible_tiles_u_set_.end())
        return;

    visible_tiles_u_set_.insert(tile_key);

    const auto x_min = -kMapBoundEpsg3857 + x_index * tile_epsg_3857_length_;
    const auto y_min = -kMapBoundEpsg3857 + y_index * tile_epsg_3857_length_;
    const auto x_max = x_min + tile_epsg_3857_length_;
    const auto y_max = y_min + tile_epsg_3857_length_;

    const auto epsg_3857_rect = projection::Epsg3857Rect(x_min, y_min, x_max, y_max);
    const auto tile = map::Tile(x_index, y_index);

    renderer_processes_manager_u_ptr_->AddRenderingTask(epsg_3857_rect, tile, zoom_);
}

void MapWidget::Zoom(const QPointF& zoom_position)
{
    UpdateMapWithNewZoom(RelativeScenePoint(zoom_position.x() / axis_tile_count_,
                                            zoom_position.y() / axis_tile_count_));
}

void MapWidget::DrawRoute()
{
    auto route_point_stack = navigation_manager_u_ptr_->FindPath(route_.start_point.epsg_3857_point_u_ptr, route_.end_point.epsg_3857_point_u_ptr);
    if (route_point_stack.empty())
        return;

    auto last_point = std::move(route_point_stack.top());
    route_point_stack.pop();

    projection::Epsg3857PointUPtr point;
    QGraphicsLineItem* line;

    for (; !route_point_stack.empty();)
    {
        point = std::move(route_point_stack.top());
        route_point_stack.pop();

        line = scene_.addLine((last_point->x - kPenWidth + kMapBoundEpsg3857) / pixel_epsg_3857_length_,
                              (kMapBoundEpsg3857 - last_point->y - kPenWidth) / pixel_epsg_3857_length_,
                              (point->x - kPenWidth + kMapBoundEpsg3857) / pixel_epsg_3857_length_,
                              (kMapBoundEpsg3857 - point->y - kPenWidth) / pixel_epsg_3857_length_,
                              QPen(Qt::red, kPenWidth * 0.75));

        route_.line_deque.push_back(line);

        last_point = std::move(point);
    }

    return;
}

void MapWidget::OnZoomInWheel(const QPointF& zoom_position)
{
    zoom_++;

    if (zoom_ > kZoomUpperBound)
    {
        zoom_ = kZoomUpperBound;
        return;
    }

    map_controls_widget_.SetCurrentZoom(zoom_);

    Zoom(zoom_position);
}

void MapWidget::OnZoomOutWheel(const QPointF& zoom_position)
{
    zoom_--;

    if (zoom_ < kZoomLowerBound)
    {
        zoom_ = kZoomLowerBound;
        return;
    }

    map_controls_widget_.SetCurrentZoom(zoom_);

    Zoom(zoom_position);
}

void MapWidget::OnZoomInButtton()
{
    zoom_++;

    const auto graphics_view_center = graphics_view_.mapToScene(graphics_view_.viewport()->rect().center());
    Zoom(graphics_view_center);
}

void MapWidget::OnZoomOutButtton()
{
    zoom_--;

    const auto graphics_view_center = graphics_view_.mapToScene(graphics_view_.viewport()->rect().center());
    Zoom(graphics_view_center);
}

void MapWidget::OnLocationButtonPressed()
{
    auto device_location_epsg_4326_point = projection::Epsg4326Point();
    if (!location::RequestDeviceLocation(device_location_epsg_4326_point))
        return;

    auto device_location_epsg_3857_point = projection::Epsg3857Point();
    if (!projection::Transform(device_location_epsg_4326_point, device_location_epsg_3857_point))
        return;

    const auto scene_x = (device_location_epsg_3857_point.x + kMapBoundEpsg3857) / tile_epsg_3857_length_ * kTilePixelSize;
    const auto scene_y = (kMapBoundEpsg3857 - device_location_epsg_3857_point.y) / tile_epsg_3857_length_ * kTilePixelSize;

    graphics_view_.centerOn(scene_x, scene_y);
}

void MapWidget::OnMapClicked(const QPointF& position)
{
    if (route_.end_point.is_set)
    {
        scene_.removeItem(route_.start_point.scene_item);
        route_.start_point.is_set = false;

        scene_.removeItem(route_.end_point.scene_item);
        route_.end_point.is_set = false;

        auto line = route_.line_deque.begin();
        for (; !route_.line_deque.empty();)
        {
            scene_.removeItem(*line);
            line = route_.line_deque.erase(line);
        }
    }

    auto nearest_road_point = std::make_unique<projection::Epsg3857Point>();
    if (!navigation_manager_u_ptr_->FindNearestRoadPoint(
            QPointF(
                (position.x() * pixel_epsg_3857_length_) - kMapBoundEpsg3857,
                (kMapBoundEpsg3857 - position.y() * pixel_epsg_3857_length_)),
            *nearest_road_point))
        return;

    const auto ellipse_item = scene_.addEllipse(
        (nearest_road_point->x - kPenWidth + kMapBoundEpsg3857) / pixel_epsg_3857_length_,
        (kMapBoundEpsg3857 - nearest_road_point->y - kPenWidth) / pixel_epsg_3857_length_,
        2 * kPenWidth,
        2 * kPenWidth,
        QPen(Qt::red),
        QBrush(Qt::red));

    if (!route_.start_point.is_set)
        route_.start_point = RoadPoint(std::move(nearest_road_point), ellipse_item);
    else
    {
        route_.end_point = RoadPoint(std::move(nearest_road_point), ellipse_item);
        DrawRoute();
    }
}

void MapWidget::OnImageRendered(std::shared_ptr<QPixmap> pixmap, const map::Tile& tile, const unsigned int zoom)
{
    /* When we change zoom and stop rendering, processes that are already
    rendering will not stop and we will receive rendered images of another
    zoom level which we will not display */
    if (zoom != zoom_)
        return;

    const auto pixmap_item = scene_.addPixmap(*pixmap);
    pixmap_item->setPos(kTilePixelSize * tile.x_index, kTilePixelSize * (max_axis_index_ - tile.y_index));
}
