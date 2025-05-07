#include <codecvt>
#include "logger.h"
#include "process_collector.h"

int main() {
    LocalWatcher::Logger::Init();

    auto logger = LocalWatcher::Logger::Get();

    logger->info("Starting process collection...");

    LocalWatcher::Collectors::ProcessCollector collector;

    auto processes = collector.CollectAll();

    logger->info("Collected %d processes", processes.size());

    for (auto& process : processes) {
        logger->info("PID {} | Name: {} | User: {} | Mem: {} KB | CPU(ms): {}",
            process.pid,
            std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(process.name),
            std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(process.user),
            process.memoryUsageBytes / 1024,
            process.cpuTime
            );
    }

    return 0;
}
