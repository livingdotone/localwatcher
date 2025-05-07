//
// Created by lucas on 07/05/2025.
//

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <minwindef.h>

namespace LocalWatcher::Collectors {

    struct ProcessInfo {
        DWORD pid;
        std::wstring name;
        std::wstring fullPath;
         std::wstring user;
        uint64_t creationTime;
        uint64_t cpuTime;
        size_t memoryUsageBytes;
    };

    class ProcessCollector {
    public:
        static std::vector<ProcessInfo> CollectAll();

        static std::optional<ProcessInfo> CollectByPid(uint32_t pid);
    };
}
