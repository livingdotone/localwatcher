//
// Created by lucas on 07/05/2025.
//

#include "logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>

namespace LocalWatcher {
    logger_ptr Logger::logger_ = nullptr;

    void Logger::Init() {
        logger_ = spdlog::stdout_color_mt("console");
        logger_->set_level(spdlog::level::info);
        logger_->set_pattern("[%H:%M:%S] [%^%l%$] %v");
    }

    logger_ptr& Logger::Get() {
        return logger_;
    }
}