#include "MapGraphicsView.h"

#include <QWheelEvent>
#include <QScrollBar>

MapGraphicsView::MapGraphicsView(QWidget *parent)
    : QGraphicsView(parent)
{

}

MapGraphicsView::MapGraphicsView(QGraphicsScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent)
{

}

void MapGraphicsView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        is_dragging_ = true;
        last_mouse_pos_ = event->pos();
        setCursor(Qt::ClosedHandCursor);

        click_timer_.start(250);
    }

    QGraphicsView::mousePressEvent(event);
}

void MapGraphicsView::mouseMoveEvent(QMouseEvent* event)
{
    if (is_dragging_) {
        const auto delta = event->pos() - last_mouse_pos_;
        last_mouse_pos_ = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());

        click_timer_.stop();
    }

    QGraphicsView::mouseMoveEvent(event);
}

void MapGraphicsView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        is_dragging_ = false;
        setCursor(Qt::ArrowCursor);

        if (click_timer_.isActive()) {
            click_timer_.stop();
            const auto click_position = mapToScene(event->pos());
            emit MapClicked(click_position);
        }
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void MapGraphicsView::wheelEvent(QWheelEvent *event)
{
    /* // Масштабирование при помощи колеса мыши
    double scaleFactor = 1.15; // Множитель масштаба
    QPoint numPixels = event->angleDelta() / 8;
    QPoint numDegrees = numPixels / 15;
    qreal numSteps = numDegrees.y();
    if (numSteps > 0) {
        scale(scaleFactor, scaleFactor);
    } else {
        scale(1.0 / scaleFactor, 1.0 / scaleFactor);
    }

    if (transform().m11() >= 2.0 || transform().m22() >= 2.0) {
        std::cout << 1 << std::endl;
    }*/

    const auto zoom_position_in_widget = event->position();
    const auto zoom_position_in_scene = mapToScene(zoom_position_in_widget.toPoint());

    if (event->angleDelta().y() > 0)
        emit ZoomIn(zoom_position_in_scene);
    else
        emit ZoomOut(zoom_position_in_scene);

    event->accept();
}
