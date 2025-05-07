//
// Created by lucas on 07/05/2025.
//

#pragma once

#include <spdlog/spdlog.h>

namespace LocalWatcher {
    using logger_ptr = std::shared_ptr<spdlog::logger>;

    class Logger {
        public:
            static void Init();

            static logger_ptr& Get();

        private:
            static logger_ptr logger_;
    };
}