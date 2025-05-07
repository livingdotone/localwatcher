#include "process_collector.h"
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

#include <vector>
#include <string>
#include <optional>

namespace LocalWatcher::Collectors {

    /**
     * Retrieves the username (domain\username) associated with a process.
     *
     * @param hProcess Handle to the process with TOKEN_QUERY rights.
     * @return Optional string in the format "DOMAIN\\Username", or std::nullopt on failure.
     */
    std::optional<std::wstring> GetProcessUser(HANDLE hProcess) {
        HANDLE hToken = nullptr;
        if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken))
            return std::nullopt;

        DWORD size = 0;
        GetTokenInformation(hToken, TokenUser, nullptr, 0, &size);
        std::vector<BYTE> buffer(size);

        if (!GetTokenInformation(hToken, TokenUser, buffer.data(), size, &size)) {
            CloseHandle(hToken);
            return std::nullopt;
        }

        const TOKEN_USER* tokenUser = reinterpret_cast<TOKEN_USER*>(buffer.data());
        WCHAR name[256], domain[256];
        DWORD nameLen = 256, domainLen = 256;
        SID_NAME_USE sidType;

        if (LookupAccountSidW(nullptr, tokenUser->User.Sid, name, &nameLen, domain, &domainLen, &sidType)) {
            CloseHandle(hToken);
            return std::wstring(domain) + L"\\" + std::wstring(name);
        }

        CloseHandle(hToken);
        return std::nullopt;
    }

    /**
     * Calculates the total CPU time (user + kernel) used by the process.
     *
     * @param hProcess Handle to the process.
     * @return CPU time in milliseconds, or std::nullopt on failure.
     */
    std::optional<uint64_t> GetProcessCPUTime(HANDLE hProcess) {
        FILETIME createTime, exitTime, kernelTime, userTime;
        if (!GetProcessTimes(hProcess, &createTime, &exitTime, &kernelTime, &userTime))
            return std::nullopt;

        ULARGE_INTEGER kTime, uTime;
        kTime.LowPart = kernelTime.dwLowDateTime;
        kTime.HighPart = kernelTime.dwHighDateTime;
        uTime.LowPart = userTime.dwLowDateTime;
        uTime.HighPart = userTime.dwHighDateTime;

        return (kTime.QuadPart + uTime.QuadPart) / 1000;
    }

    /**
     * Retrieves the process creation timestamp.
     *
     * @param hProcess Handle to the process.
     * @return Creation time in FILETIME format (UTC), or std::nullopt on failure.
     */
    std::optional<uint64_t> GetProcessCreationTime(HANDLE hProcess) {
        FILETIME createTime, exitTime, kernelTime, userTime;
        if (!GetProcessTimes(hProcess, &createTime, &exitTime, &kernelTime, &userTime))
            return std::nullopt;

        ULARGE_INTEGER cTime;
        cTime.LowPart = createTime.dwLowDateTime;
        cTime.HighPart = createTime.dwHighDateTime;

        return cTime.QuadPart;
    }

    /**
     * Collects basic information about all accessible processes in the system.
     *
     * @return Vector of ProcessInfo structures for each successfully queried process.
     */
    std::vector<ProcessInfo> ProcessCollector::CollectAll() {
        std::vector<ProcessInfo> processes;

        HANDLE snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshotHandle == INVALID_HANDLE_VALUE)
            return processes;

        PROCESSENTRY32W entry;
        entry.dwSize = sizeof(entry);

        if (!Process32FirstW(snapshotHandle, &entry)) {
            CloseHandle(snapshotHandle);
            return processes;
        }

        do {
            auto processInfo = CollectByPid(entry.th32ProcessID);
            if (processInfo.has_value()) {
                processes.push_back(std::move(processInfo.value()));
            }
        } while (Process32NextW(snapshotHandle, &entry));

        CloseHandle(snapshotHandle);
        return processes;
    }

    /**
     * Collects detailed information about a specific process by PID.
     *
     * This includes executable name, full path, memory usage,
     * user identity, creation time, and total CPU usage.
     *
     * @param pid Process ID to inspect.
     * @return ProcessInfo if successful, or std::nullopt if access fails.
     */
    std::optional<ProcessInfo> ProcessCollector::CollectByPid(uint32_t pid) {
        HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (!processHandle)
            return std::nullopt;

        WCHAR path[MAX_PATH] = L"<unknown>";
        if (!GetModuleFileNameExW(processHandle, nullptr, path, MAX_PATH)) {
            CloseHandle(processHandle);
            return std::nullopt;
        }

        PROCESS_MEMORY_COUNTERS pmc;
        if (!GetProcessMemoryInfo(processHandle, &pmc, sizeof(pmc))) {
            CloseHandle(processHandle);
            return std::nullopt;
        }

        auto optUser = GetProcessUser(processHandle);
        auto optCreationTime = GetProcessCreationTime(processHandle);
        auto optCPUTime = GetProcessCPUTime(processHandle);

        if (!optUser || !optCreationTime || !optCPUTime) {
            CloseHandle(processHandle);
            return std::nullopt;
        }

        ProcessInfo info{
            pid,
            std::wstring(path).substr(std::wstring(path).find_last_of(L"\\") + 1), // Extract name
            std::wstring(path),
            optUser.value(),
            optCreationTime.value(),
            optCPUTime.value(),
            static_cast<size_t>(pmc.WorkingSetSize)
        };

        CloseHandle(processHandle);
        return info;
    }

}