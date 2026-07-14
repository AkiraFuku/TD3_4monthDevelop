#include "PerformanceMonitor.h"
#include <Windows.h>
#include <Psapi.h>
#include <algorithm>
#include <chrono>
#include <format>
#include <fstream>
#include <string>
#pragma comment(lib, "Psapi.lib")

namespace {
constexpr double kSampleIntervalSeconds = 0.5;
constexpr double kBytesPerMiB = 1024.0 * 1024.0;
}

PerformanceMonitor::~PerformanceMonitor() { Stop(); }

void PerformanceMonitor::Start() {
    if (monitorThread_.joinable()) { return; }
    {
        std::lock_guard lock(frameStatsMutex_);
        completedFrameCount_ = 0;
        totalFrameTimeMs_ = 0.0;
        maxFrameTimeMs_ = 0.0;
    }
    monitorThread_ = std::jthread([this](std::stop_token token) { Monitor(token); });
}

void PerformanceMonitor::Stop() {
    if (monitorThread_.joinable()) {
        monitorThread_.request_stop();
        monitorThread_.join();
    }
}

void PerformanceMonitor::NotifyFrameCompleted(double frameTimeMs) {
    std::lock_guard lock(frameStatsMutex_);
    ++completedFrameCount_;
    totalFrameTimeMs_ += frameTimeMs;
    maxFrameTimeMs_ = (std::max)(maxFrameTimeMs_, frameTimeMs);
}

void PerformanceMonitor::SetSceneName(const std::string& sceneName) {
    std::lock_guard lock(sceneNameMutex_);
    sceneName_ = sceneName.empty() ? "Unknown" : sceneName;
}

void PerformanceMonitor::Monitor(std::stop_token stopToken) {
    const std::string filePath = "logs/performance_" +
        std::to_string(GetCurrentProcessId()) + ".csv";
    std::ofstream output(filePath, std::ios::trunc);
    output << "elapsed_seconds,scene,fps,frame_time_ms,max_frame_time_ms,working_set_mib,private_memory_mib\n";

    const auto monitorStart = std::chrono::steady_clock::now();
    auto previousSample = monitorStart;
    while (!stopToken.stop_requested()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        const auto now = std::chrono::steady_clock::now();
        const double sampleSeconds = std::chrono::duration<double>(now - previousSample).count();
        if (sampleSeconds < kSampleIntervalSeconds) { continue; }

        std::uint64_t frames = 0;
        double totalFrameTimeMs = 0.0;
        double maxFrameTimeMs = 0.0;
        {
            std::lock_guard lock(frameStatsMutex_);
            frames = completedFrameCount_;
            totalFrameTimeMs = totalFrameTimeMs_;
            maxFrameTimeMs = maxFrameTimeMs_;
            completedFrameCount_ = 0;
            totalFrameTimeMs_ = 0.0;
            maxFrameTimeMs_ = 0.0;
        }
        const double fps = frames / sampleSeconds;
        const double frameTimeMs = frames > 0 ? totalFrameTimeMs / frames : 0.0;
        PROCESS_MEMORY_COUNTERS_EX memoryCounters{};
        memoryCounters.cb = sizeof(memoryCounters);
        GetProcessMemoryInfo(GetCurrentProcess(),
            reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memoryCounters), sizeof(memoryCounters));

        std::string sceneName;
        {
            std::lock_guard lock(sceneNameMutex_);
            sceneName = sceneName_;
        }

        const double elapsedSeconds = std::chrono::duration<double>(now - monitorStart).count();
        output << elapsedSeconds << ',' << sceneName << ',' << fps << ',' << frameTimeMs << ','
            << maxFrameTimeMs << ','
            << memoryCounters.WorkingSetSize / kBytesPerMiB << ','
            << memoryCounters.PrivateUsage / kBytesPerMiB << '\n';
        output.flush();
        OutputDebugStringA(std::format(
            "[Performance] FPS: {:.1f}, Frame avg/max: {:.2f}/{:.2f} ms, Working set: {:.1f} MiB, Private: {:.1f} MiB\n",
            fps, frameTimeMs, maxFrameTimeMs, memoryCounters.WorkingSetSize / kBytesPerMiB,
            memoryCounters.PrivateUsage / kBytesPerMiB).c_str());
        previousSample = now;
    }
}
