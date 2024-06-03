#ifndef RENDERERPROCESSESMANAGER_H
#define RENDERERPROCESSESMANAGER_H

#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

#include <QProcess>

#include "Projection.h"
#include "Map.h"

class RendererProcessesManager;
using RendererProcessesManagerUPtr = std::unique_ptr<RendererProcessesManager>;

/* This class designed to create processes for rendering and manage them.
We cannot use multithreading in one application because library used for
rendering (mapnik) can use only one connection to database simultaneously.
Which leads to errors when using multiple threads for rendering in one
process */
class RendererProcessesManager : public QObject
{
    Q_OBJECT

public:
    static RendererProcessesManagerUPtr Create(const unsigned int process_count, QObject* parent = nullptr);
    ~RendererProcessesManager();

    //! Adds rendering task in queue. After rendering completion ImageRendered signal will be emitted
    void AddRenderingTask(const projection::Epsg3857Rect& epsg_3857_rect, const map::Tile& tile, const unsigned int zoom);
    //! Clears the queue of rendering tasks, does not stop those that are already running
    void ClearRenderingTasks();

signals:
    void ImageRendered(std::shared_ptr<QPixmap> pixmap, const map::Tile& tile, const unsigned int zoom);

public slots:
    void OnRenderingFinish();

private:
    struct RenderingTask
    {
        RenderingTask(const projection::Epsg3857Rect& epsg_3857_rect, const map::Tile& tile, const unsigned int zoom)
            : epsg_3857_rect(epsg_3857_rect), tile(tile), zoom(zoom)
        {

        }

        projection::Epsg3857Rect epsg_3857_rect;
        map::Tile tile;
        unsigned int zoom;
    };

    using RenderingTaskUPtr = std::unique_ptr<RenderingTask>;

    std::mutex rendering_task_queue_mutex_;
    std::queue<RenderingTaskUPtr> rendering_task_queue_;
    std::condition_variable rendering_task_added_or_thread_stop_cv_;

    std::atomic<bool> rendering_task_queue_manager_thread_stop_ = false;
    std::thread rendering_task_queue_manager_thread_;

    std::mutex free_process_pool_mutex_;
    std::queue<QProcess*> free_process_pool_;
    std::condition_variable process_released_or_thread_stop_cv_;

    RendererProcessesManager(QObject* parent = nullptr);

    void StartManagingRenderingTaskQueue();
    void SendDataToRendererProcess(const RenderingTaskUPtr rendering_task);

    static QProcess* CreateProcess(const unsigned int process_index, RendererProcessesManager* instance);

    QProcess* AcquireProcess();
    void ReleaseProcess(QProcess* process);
};

#endif // RENDERERPROCESSESMANAGER_H
