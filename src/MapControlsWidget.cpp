#include "MapControlsWidget.h"

#include <QVBoxLayout>

constexpr char kIconPath[] = "icon/";

MapControlsWidget::MapControlsWidget(QWidget *parent)
    : QWidget(parent)
{
    const auto plus_icon = QIcon(QString("%1plus.png").arg(kIconPath));
    zoom_in_button_ = new QPushButton(this);
    zoom_in_button_->setToolTip("Zoom in");
    connect(zoom_in_button_, &QPushButton::clicked, this, &MapControlsWidget::OnZoomInButtonPress);
    zoom_in_button_->setFixedSize(30, 30);
    zoom_in_button_->setIcon(plus_icon);

    const auto minus_icon = QIcon(QString("%1minus.webp").arg(kIconPath));
    zoom_out_button_ = new QPushButton(this);
    zoom_out_button_->setToolTip("Zoom out");
    zoom_out_button_->setDisabled(true);
    connect(zoom_out_button_, &QPushButton::clicked, this, &MapControlsWidget::OnZoomOutButtonPress);
    zoom_out_button_->setFixedSize(30, 30);
    zoom_out_button_->setIcon(minus_icon);

    const auto location_icon = QIcon(QString("%1location.webp").arg(kIconPath));
    location_button_ = new QPushButton(this);
    location_button_->setToolTip("Center on my location");
    connect(location_button_, &QPushButton::clicked, this, [this]() { emit LocationButtonPressed(); } );
    location_button_->setFixedSize(30, 30);
    location_button_->setIcon(location_icon);

    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignCenter);

    layout->addWidget(zoom_in_button_);
    layout->addWidget(zoom_out_button_);
    layout->addWidget(location_button_);

    setLayout(layout);
}

void MapControlsWidget::SetZoomRange(const unsigned int min_zoom, const unsigned int max_zoom)
{
    min_zoom_ = min_zoom;
    max_zoom_ = max_zoom;

    CheckZoom();
}

void MapControlsWidget::SetCurrentZoom(const unsigned int zoom)
{
    current_zoom_ = zoom;

    CheckZoom();
}

void MapControlsWidget::OnZoomInButtonPress()
{
    current_zoom_++;

    zoom_out_button_->setEnabled(true);

    if (current_zoom_ >= max_zoom_)
        zoom_in_button_->setDisabled(true);

    emit ZoomIn();
}

void MapControlsWidget::OnZoomOutButtonPress()
{
    current_zoom_--;

    zoom_in_button_->setEnabled(true);

    if (current_zoom_ <= min_zoom_)
        zoom_out_button_->setDisabled(true);

    emit ZoomOut();
}

void MapControlsWidget::CheckZoom()
{
    if (current_zoom_ > min_zoom_)
        zoom_out_button_->setEnabled(true);
    else
        zoom_out_button_->setDisabled(true);

    if (current_zoom_ < max_zoom_)
        zoom_in_button_->setEnabled(true);
    else
        zoom_in_button_->setDisabled(true);
}
