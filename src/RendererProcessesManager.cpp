#include "RendererProcessesManager.h"

#include <iostream>
#include <sstream>

#include <QVariant>
#include <QPixmap>

RendererProcessesManager::RendererProcessesManager(QObject* parent)
    : QObject(parent)
{

}

RendererProcessesManagerUPtr RendererProcessesManager::Create(const unsigned int process_count, QObject* parent)
{
    std::unique_ptr<RendererProcessesManager> instance(new RendererProcessesManager(parent));

    for (auto i = 0; i < process_count; i++)
    {
        const auto process = CreateProcess(i, instance.get());
        if (!process)
            return nullptr;

        instance->free_process_pool_.emplace(std::move(process));
    }

    instance->rendering_task_queue_manager_thread_ = std::thread([instance = instance.get()]() { instance->StartManagingRenderingTaskQueue(); });

    return instance;
}

RendererProcessesManager::~RendererProcessesManager()
{
    rendering_task_queue_manager_thread_stop_.store(true);
    rendering_task_added_or_thread_stop_cv_.notify_one();
    process_released_or_thread_stop_cv_.notify_one();

    if (rendering_task_queue_manager_thread_.joinable())
        rendering_task_queue_manager_thread_.join();
}

void RendererProcessesManager::SendDataToRendererProcess(const RenderingTaskUPtr rendering_task)
{
    auto process = AcquireProcess();
    if (!process)
        return;

    const auto write_to_process = [&, this](const auto& value) {
        process->write(QByteArray::number(value));
        process->write("\n");
    };

    write_to_process(rendering_task->epsg_3857_rect.bottom_left_point.x);
    write_to_process(rendering_task->epsg_3857_rect.bottom_left_point.y);
    write_to_process(rendering_task->epsg_3857_rect.top_right_point.x);
    write_to_process(rendering_task->epsg_3857_rect.top_right_point.y);

    write_to_process(rendering_task->tile.x_index);
    write_to_process(rendering_task->tile.y_index);

    write_to_process(rendering_task->zoom);

    process->waitForBytesWritten();
}

void RendererProcessesManager::OnRenderingFinish()
{
    const auto process = qobject_cast<QProcess*>(sender());

    const auto pixmap = std::make_shared<QPixmap>();
    pixmap->load(QString("renderer/renderer_process%1_output.png")
                     .arg(process->property("process_index").toString()));

    const auto byte_array = process->readAllStandardOutput();
    auto string_stream = std::istringstream(byte_array.toStdString());

    unsigned int x_index, y_index, zoom;
    string_stream >> x_index >> y_index >> zoom;

    const auto tile = map::Tile(map::Tile(x_index, y_index));

    emit ImageRendered(pixmap, tile, zoom);

    ReleaseProcess(process);
}

void RendererProcessesManager::AddRenderingTask(const projection::Epsg3857Rect& epsg_3857_rect, const map::Tile& tile, const unsigned int zoom)
{
    std::lock_guard rendering_task_queue_lock(rendering_task_queue_mutex_);
    rendering_task_queue_.push(std::make_unique<RenderingTask>(epsg_3857_rect, tile, zoom));

    rendering_task_added_or_thread_stop_cv_.notify_one();
}

void RendererProcessesManager::ClearRenderingTasks()
{
    std::lock_guard rendering_task_queue_lock(rendering_task_queue_mutex_);
    for (; !rendering_task_queue_.empty();)
        rendering_task_queue_.pop();
}

void RendererProcessesManager::StartManagingRenderingTaskQueue()
{
    for (; !rendering_task_queue_manager_thread_stop_.load();)
    {
        std::unique_lock rendering_task_queue_lock(rendering_task_queue_mutex_);
        rendering_task_added_or_thread_stop_cv_.wait(rendering_task_queue_lock, [this]() {
            return !rendering_task_queue_.empty() || rendering_task_queue_manager_thread_stop_.load();
        });

        if (rendering_task_queue_manager_thread_stop_.load())
            break;

        auto rendering_task = std::move(rendering_task_queue_.front());
        rendering_task_queue_.pop();

        rendering_task_queue_lock.unlock();
        rendering_task_queue_lock.release();

        SendDataToRendererProcess(std::move(rendering_task));
    }

    ClearRenderingTasks();
}

QProcess* RendererProcessesManager::AcquireProcess()
{
    std::unique_lock<std::mutex> free_process_pool_lock(free_process_pool_mutex_);

    process_released_or_thread_stop_cv_.wait(free_process_pool_lock, [this]() {
        return !free_process_pool_.empty() || rendering_task_queue_manager_thread_stop_.load();
    });

    if (rendering_task_queue_manager_thread_stop_.load())
        return nullptr;

    const auto process = free_process_pool_.front();
    free_process_pool_.pop();

    return process;
}

void RendererProcessesManager::ReleaseProcess(QProcess* process)
{
    std::lock_guard<std::mutex> lock(free_process_pool_mutex_);
    free_process_pool_.emplace(process);

    process_released_or_thread_stop_cv_.notify_one();
}

QProcess* RendererProcessesManager::CreateProcess(const unsigned int process_index, RendererProcessesManager* instance)
{
    auto process = new QProcess(instance);
    process->setProperty("process_index", process_index);

    const auto program = "renderer/Renderer";
    const auto arguments = QStringList(QString::number(process_index));

    process->start(program, arguments);

    if (!process->waitForStarted())
    {
        std::cerr << "RendererProcessesManager::CreateProcess Failed to start process" << std::endl;
        return nullptr;
    }

    connect(process, &QProcess::readyReadStandardOutput, instance, &RendererProcessesManager::OnRenderingFinish);

    return process;
}
