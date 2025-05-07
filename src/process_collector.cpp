//
// Created by lucas on 07/05/2025.
//

#include "process_collector.h"
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <sddl.h>

#include <vector>
#include <string>
#include <optional>

namespace LocalWatcher::Collectors {
    std::optional<std::wstring> GetProcessUser(HANDLE hProcess) {
        HANDLE hToken = nullptr;
        if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) {
            return std::nullopt;
        }

        DWORD size = 0;
        GetTokenInformation(hToken, TokenUser, nullptr, 0, &size);
        std::vector<BYTE> buffer(size);

        if (!GetTokenInformation(hToken, TokenUser, &buffer[0], size, &size)) {
            CloseHandle(hToken);
            return std::nullopt;
        }

        auto* tokenUser = reinterpret_cast<TOKEN_USER*>(buffer.data());
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

    std::optional<uint64_t> GetProcessCPUTime(HANDLE hProcess) {
        FILETIME createTime, exitTime, kernelTime, userTime;
        if (!GetProcessTimes(hProcess, &createTime, &exitTime, &kernelTime, &userTime)) return std::nullopt;

        ULARGE_INTEGER kTime, uTime;
        kTime.LowPart = kernelTime.dwLowDateTime;
        kTime.HighPart = kernelTime.dwHighDateTime;
        uTime.LowPart = userTime.dwLowDateTime;
        uTime.HighPart = userTime.dwHighDateTime;

        return (kTime.QuadPart + uTime.QuadPart) / 1000;
    }

    std::optional<uint64_t> GetProcessCreationTime(HANDLE hProcess) {
        FILETIME createTime, exitTime, kernelTime, userTime;

        if (!GetProcessTimes(hProcess, &createTime, &exitTime, &kernelTime, &userTime)) return std::nullopt;

        ULARGE_INTEGER cTime;
        cTime.LowPart = userTime.dwLowDateTime;
        cTime.HighPart = userTime.dwHighDateTime;

        return cTime.QuadPart;
    }

    std::vector<ProcessInfo> ProcessCollector::CollectAll() {
        std::vector<ProcessInfo> processes;

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) return processes;

        PROCESSENTRY32W entry;
        entry.dwSize = sizeof(PROCESSENTRY32);

        if (!Process32FirstW(snapshot, &entry)) {
            CloseHandle(snapshot);
            return processes;
        }

        do {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, entry.th32ProcessID);

            if (!hProcess) continue;

            WCHAR path[MAX_PATH] = L"<unknown>";
            GetModuleFileNameExW(hProcess, nullptr, path, MAX_PATH);

            PROCESS_MEMORY_COUNTERS pmc;
            GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc));

            auto optProcessUser = GetProcessUser(&hProcess);
            auto optProcessCreationTime = GetProcessCreationTime(&hProcess);
            auto optProcessCPUTime = GetProcessCPUTime(&hProcess);

            if (!optProcessUser.has_value()) {
                CloseHandle(hProcess);
                continue;
            }



            processes.push_back(ProcessInfo{
                entry.th32ProcessID,
                std::wstring(entry.szExeFile),
                std::wstring(path),
                optProcessUser.value(),
                optProcessCreationTime.value(),
                optProcessCPUTime.value(),
                pmc.WorkingSetSize
            });
        } while (Process32NextW(snapshot, &entry));

        CloseHandle(snapshot);
        return processes;
    }

    std::optional<ProcessInfo> ProcessCollector::CollectByPid(uint32_t pid) {
        auto process = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, pid);

        if (process == INVALID_HANDLE_VALUE) return std::nullopt;

        PROCESSENTRY32W entry;
        entry.dwSize = sizeof(PROCESSENTRY32);

        if (!Process32FirstW(process, &entry)) {
            CloseHandle(process);
            return std::nullopt;
        }

        WCHAR path[MAX_PATH] = L"<unknown>";
        GetModuleFileNameExW(process, nullptr, path, MAX_PATH);

        PROCESS_MEMORY_COUNTERS pmc;
        GetProcessMemoryInfo(process, &pmc, sizeof(pmc));

        auto optProcessUser = GetProcessUser(&process);
        auto optProcessCreationTime = GetProcessCreationTime(&process);
        auto optProcessCPUTime = GetProcessCPUTime(&process);

        if (!optProcessUser.has_value()) {
            CloseHandle(process);
            return std::nullopt;
        };


        return ProcessInfo{
            entry.th32ProcessID,
            std::wstring(entry.szExeFile),
            std::wstring(path),
            optProcessUser.value(),
            optProcessCreationTime.value(),
            optProcessCPUTime.value(),
            pmc.WorkingSetSize
        };
    }


}