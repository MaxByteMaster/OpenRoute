#ifndef MAPCONTROLSWIDGET_H
#define MAPCONTROLSWIDGET_H

#include <QWidget>
#include <QPushButton>

class MapControlsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MapControlsWidget(QWidget* parent = nullptr);

    void SetZoomRange(const unsigned int min_zoom, const unsigned int max_zoom);
    void SetCurrentZoom(const unsigned int zoom);

signals:
    void ZoomIn();
    void ZoomOut();
    void LocationButtonPressed();

private slots:
    void OnZoomInButtonPress();
    void OnZoomOutButtonPress();

private:
    unsigned int current_zoom_ = 0;

    unsigned int min_zoom_ = 0;
    unsigned int max_zoom_ = UINT_MAX;

    QPushButton* zoom_in_button_;
    QPushButton* zoom_out_button_;
    QPushButton* location_button_;

    void CheckZoom();
};

#endif // MAPCONTROLSWIDGET_H
