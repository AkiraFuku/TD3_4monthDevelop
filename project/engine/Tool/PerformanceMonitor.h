#pragma once
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

class PerformanceMonitor {
public:
    ~PerformanceMonitor();
    void Start();
    void Stop();
    void NotifyFrameCompleted(double frameTimeMs);
    void SetSceneName(const std::string& sceneName);

private:
    void Monitor(std::stop_token stopToken);
    std::jthread monitorThread_;
    std::mutex frameStatsMutex_;
    std::uint64_t completedFrameCount_ = 0;
    double totalFrameTimeMs_ = 0.0;
    double maxFrameTimeMs_ = 0.0;
    std::mutex sceneNameMutex_;
    std::string sceneName_ = "Unknown";
};
