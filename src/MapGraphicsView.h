#ifndef MAPGRAPHICSVIEW_H
#define MAPGRAPHICSVIEW_H

#include <QTimer>
#include <QGraphicsView>

class MapGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    MapGraphicsView(QWidget* parent = nullptr);
    MapGraphicsView(QGraphicsScene* scene, QWidget* parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    bool is_dragging_;
    QPoint last_mouse_pos_;
    QTimer click_timer_;

signals:
    void ZoomIn(const QPointF& zoom_position);
    void ZoomOut(const QPointF& zoom_position);
    void MapClicked(const QPointF& position);
};

#endif // MAPGRAPHICSVIEW_H
