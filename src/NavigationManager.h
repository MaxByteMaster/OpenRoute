#ifndef NAVIGATIONMANAGER_H
#define NAVIGATIONMANAGER_H

#include <stack>

#include <QPoint>
#include <QSqlDatabase>

#include "Projection.h"

class NavigationManager;
typedef std::unique_ptr<NavigationManager> NavigationManagerUPtr;

class NavigationManager
{
public:
    static NavigationManagerUPtr Create();

    bool FindNearestRoadPoint(const QPointF& position, projection::Epsg3857Point& nearest_road_point);
    std::stack<projection::Epsg3857PointUPtr> FindPath(projection::Epsg3857PointUPtr& start_epsg_3857_point, projection::Epsg3857PointUPtr& end_epsg_3857_point_u_ptr);

private:
    QSqlDatabase database_;

    NavigationManager();
};

#endif // NAVIGATIONMANAGER_H
