#include "NavigationManager.h"

#include <iostream>

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtSql/QSqlDatabase>
#include <QRegularExpression>

NavigationManager::NavigationManager()
{
    database_ = QSqlDatabase::addDatabase("QPSQL");
    database_.setHostName("localhost");
    database_.setDatabaseName("gis");
    database_.setUserName("mapper");
    database_.setPassword("");
}

NavigationManagerUPtr NavigationManager::Create()
{
    auto instance = std::make_unique<NavigationManager>(NavigationManager());

    if (!instance->database_.open())
    {
        std::cerr << "NavigationManager::Create Database connection error: " << instance->database_.lastError().text().toStdString() << std::endl;
        return nullptr;
    }

    return instance;
}

bool NavigationManager::FindNearestRoadPoint(const QPointF& position, projection::Epsg3857Point& nearest_road_point)
{
    QSqlQuery query;
    query.prepare(R"(
        SELECT
            ST_AsText(ST_ClosestPoint(way, ST_SetSRID(ST_MakePoint(:x, :y), 3857))) AS closest_point,
            ST_Distance(way, ST_SetSRID(ST_MakePoint(:x, :y), 3857)) AS distance
        FROM
            planet_osm_line
        WHERE
            highway IS NOT NULL
        ORDER BY
            distance
        LIMIT 1;
    )");
    query.bindValue(":x", position.x());
    query.bindValue(":y", position.y());

    if (!query.exec() || !query.next())
    {
        std::cerr << "NavigationManager::FindNearestRoadPoint SQL query execution error: " << query.lastError().text().toStdString() << std::endl;
        return false;
    }

    const auto point_string = query.value(0).toString();

    QRegularExpression regex("POINT\\(([-\\d\\.]+) ([-\\d\\.]+)\\)");
    QRegularExpressionMatch match = regex.match(point_string);
    if (!match.hasMatch())
    {
        std::cerr << "NavigationManager::FindNearestRoadPoint Failed to parse string for coordinates: " << point_string.toStdString() << std::endl;
        return false;
    }

    nearest_road_point.x = match.captured(1).toDouble();
    nearest_road_point.y = match.captured(2).toDouble();

    return true;
}

struct Vertice;
using VerticeSPtr = std::shared_ptr<Vertice>;

struct Vertice
{
    int64_t id;
    VerticeSPtr previous_vertice = nullptr;
    projection::Epsg3857PointUPtr epsg_3857_point_u_ptr;
    double cost = std::numeric_limits<double>::max();
};

struct NeighbourVertice
{
    double distance;
    VerticeSPtr vertice;
};

using NeighbourVerticeSPtr = std::shared_ptr<NeighbourVertice>;

double EstimateDistance(VerticeSPtr start_vertice, VerticeSPtr end_vertice)
{
    double dx = start_vertice->epsg_3857_point_u_ptr->x - end_vertice->epsg_3857_point_u_ptr->x;
    double dy = start_vertice->epsg_3857_point_u_ptr->y - end_vertice->epsg_3857_point_u_ptr->y;
    return std::sqrt(dx * dx + dy * dy);
}

bool FindNearestVertice(const projection::Epsg3857Point& epsg_3857_point, VerticeSPtr vertice)
{
    auto query = QSqlQuery();
    query.prepare(R"(
        SELECT
            rvp.id,
            st_x(rvp.the_geom) as vertice_x,
            st_y(rvp.the_geom) as vertice_y,
            ST_Distance(r.geom, ST_SetSRID(ST_MakePoint(:x, :y), 3857)) AS distance
        FROM
            roads r
        join
            roads_vertices_pgr rvp
        on
            rvp.id = r.source
        ORDER BY
            distance
        LIMIT 1;
    )");
    query.bindValue(":x", epsg_3857_point.x);
    query.bindValue(":y", epsg_3857_point.y);

    if (!query.exec() || !query.next())
    {
        std::cerr << "NavigationManager::FindNearestVertice SQL query execution error: " << query.lastError().text().toStdString() << std::endl;
        return false;
    }

    vertice->id = query.value(0).toLongLong();

    vertice->epsg_3857_point_u_ptr = std::make_unique<projection::Epsg3857Point>();
    vertice->epsg_3857_point_u_ptr->x = query.value(1).toDouble();
    vertice->epsg_3857_point_u_ptr->y = query.value(2).toDouble();

    return true;
}

bool GetNeighbourVertices(const VerticeSPtr vertice, std::stack<NeighbourVerticeSPtr>& neighbour_vertice_stack)
{
    QSqlQuery query;
    query.prepare(R"(
        SELECT
            w.target as neighbour_id,
            st_x(wvp.the_geom) as neighbour_x,
            st_y(wvp.the_geom) as neighbour_y,
            w.length as distance_to_neighbour
        FROM
            roads w
        JOIN
            roads_vertices_pgr wvp
        ON
            wvp.id = w.target
        WHERE
            w.source = :node_id;
    )");
    query.bindValue(":node_id", QVariant::fromValue(vertice->id));

    if (!query.exec())
    {
        std::cerr << "NavigationManager::GetNeighbourVertices SQL query execution error: " << query.lastError().text().toStdString() << std::endl;
        return false;
    }

    NeighbourVerticeSPtr neighbour_vertice;
    for (; query.next();)
    {
        neighbour_vertice = std::make_shared<NeighbourVertice>();
        neighbour_vertice->vertice = std::make_shared<Vertice>();

        neighbour_vertice->vertice->id = query.value(0).toLongLong();

        neighbour_vertice->vertice->epsg_3857_point_u_ptr = std::make_unique<projection::Epsg3857Point>();
        neighbour_vertice->vertice->epsg_3857_point_u_ptr->x = query.value(1).toDouble();
        neighbour_vertice->vertice->epsg_3857_point_u_ptr->y = query.value(2).toDouble();
        neighbour_vertice->distance = query.value(3).toDouble();

        neighbour_vertice_stack.push(neighbour_vertice);
    }

    return true;
}

std::stack<VerticeSPtr> BuildPath(VerticeSPtr end_vertice)
{
    auto path = std::stack<VerticeSPtr>();

    for (; end_vertice != nullptr; )
    {
        path.push(end_vertice);
        end_vertice = end_vertice->previous_vertice;
    }

    return path;
}

VerticeSPtr PickBestVertice(std::unordered_map<int64_t, VerticeSPtr> reachable, VerticeSPtr end_vertice)
{
    auto min_cost = UINT_MAX;
    VerticeSPtr best_vertice = nullptr;

    for (const auto& vertice : reachable)
    {
        const auto cost_start_to_vertice = vertice.second->cost;
        const auto cost_vertice_to_goal = EstimateDistance(vertice.second, end_vertice);
        const auto total_cost = cost_start_to_vertice + cost_vertice_to_goal;

        if (total_cost < min_cost)
        {
            min_cost = total_cost;
            best_vertice = vertice.second;
        }
    }

    return best_vertice;
}

std::stack<VerticeSPtr> FindPath(VerticeSPtr start_vertice, VerticeSPtr end_vertice)
{
    auto reachable_u_map = std::unordered_map<int64_t, VerticeSPtr>();
    reachable_u_map[start_vertice->id] = start_vertice;

    auto explored_u_map = std::unordered_map<int64_t, VerticeSPtr>();

    VerticeSPtr vertice = nullptr;
    NeighbourVerticeSPtr neighbour;
    double from_start_node_to_neighbour_cost;
    std::stack<NeighbourVerticeSPtr> neighbour_vertice_stack;
    for (; !reachable_u_map.empty(); )
    {
        vertice = PickBestVertice(reachable_u_map, end_vertice);
        if (!vertice)
            return std::stack<VerticeSPtr>();

        if (vertice->id == end_vertice->id)
            return BuildPath(vertice);

        reachable_u_map.erase(vertice->id);
        explored_u_map[vertice->id] = vertice;

        neighbour_vertice_stack = std::stack<NeighbourVerticeSPtr>();
        if (!GetNeighbourVertices(vertice, neighbour_vertice_stack))
            return std::stack<VerticeSPtr>();

        // Fill reachable with new neighbors and compute their cost
        for (; !neighbour_vertice_stack.empty();)
        {
            neighbour = neighbour_vertice_stack.top();
            neighbour_vertice_stack.pop();

            if (explored_u_map.find(neighbour->vertice->id) != explored_u_map.end()
                || reachable_u_map.find(neighbour->vertice->id) != reachable_u_map.end())
                continue;

            reachable_u_map[neighbour->vertice->id] = neighbour->vertice;

            from_start_node_to_neighbour_cost = vertice->cost + neighbour->distance;
            if (from_start_node_to_neighbour_cost < neighbour->vertice->cost)
            {
                neighbour->vertice->cost = from_start_node_to_neighbour_cost;
                neighbour->vertice->previous_vertice = vertice;
            }
        }
    }

    return std::stack<VerticeSPtr>();
}

std::stack<projection::Epsg3857PointUPtr> NavigationManager::FindPath(projection::Epsg3857PointUPtr& start_epsg_3857_point_u_ptr, projection::Epsg3857PointUPtr& end_epsg_3857_point_u_ptr)
{
    auto start_road_vertice = std::make_shared<Vertice>();
    start_road_vertice->cost = 0;
    FindNearestVertice(*start_epsg_3857_point_u_ptr, start_road_vertice);

    auto end_road_vertice = std::make_shared<Vertice>();
    FindNearestVertice(*end_epsg_3857_point_u_ptr, end_road_vertice);

    auto road_vertice_stack = ::FindPath(start_road_vertice, end_road_vertice);

    auto full_road_vertice_stack = std::stack<projection::Epsg3857PointUPtr>();
    full_road_vertice_stack.push(std::move(start_epsg_3857_point_u_ptr));
    VerticeSPtr vertice;
    for (; !road_vertice_stack.empty();)
    {
        vertice = road_vertice_stack.top();
        road_vertice_stack.pop();

        full_road_vertice_stack.push(std::move(vertice->epsg_3857_point_u_ptr));
    }
    full_road_vertice_stack.push(std::move(end_epsg_3857_point_u_ptr));

    return full_road_vertice_stack;
}
