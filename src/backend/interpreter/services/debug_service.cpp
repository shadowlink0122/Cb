#include "services/debug_service.h"
#include <cstdio>
#include <iomanip>
#include <iostream>

DebugService &DebugService::instance() {
    static DebugService instance;
    return instance;
}

void DebugService::set_category_enabled(Category category, bool enabled) {
    category_enabled_[category] = enabled;
}

void DebugService::set_output_file(const std::string &filename) {
    if (!filename.empty()) {
        log_file_ = std::make_unique<std::ofstream>(filename, std::ios::app);
    } else {
        log_file_.reset();
    }
}

bool DebugService::should_log(Level level, Category category) const {
    if (!debug_enabled_)
        return false;
    if (level < current_level_)
        return false;

    auto category_it = category_enabled_.find(category);
    if (category_it != category_enabled_.end() && !category_it->second) {
        return false;
    }

    return true;
}

void DebugService::output_message(Level level, Category category,
                                  const std::string &message) {
    std::ostringstream oss;

    // タイムスタンプ
    if (timestamp_enabled_) {
        oss << "[" << get_timestamp() << "] ";
    }

    // レベルとカテゴリ
    oss << "[" << level_to_string(level) << "] ";
    oss << "[" << category_to_string(category) << "] ";

    // メッセージ
    oss << message;

    std::string formatted_message = oss.str();

    // コンソール出力
    if (level >= Level::WARN) {
        std::cerr << formatted_message << std::endl;
    } else {
        std::cout << formatted_message << std::endl;
    }

    // ファイル出力
    if (log_file_ && log_file_->is_open()) {
        *log_file_ << formatted_message << std::endl;
        log_file_->flush();
    }
}

void DebugService::update_stats(Category category) {
    stats_.message_counts[category]++;
    stats_.total_messages++;
}

std::string DebugService::level_to_string(Level level) const {
    switch (level) {
    case Level::TRACE:
        return "TRACE";
    case Level::DEBUG_LEVEL:
        return "DEBUG";
    case Level::INFO:
        return "INFO ";
    case Level::WARN:
        return "WARN ";
    case Level::ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

std::string DebugService::category_to_string(Category category) const {
    switch (category) {
    case Category::GENERAL:
        return "GEN";
    case Category::EXPRESSION:
        return "EXP";
    case Category::VARIABLE:
        return "VAR";
    case Category::ARRAY:
        return "ARR";
    case Category::STRUCT:
        return "STR";
    case Category::FUNCTION:
        return "FUN";
    case Category::PARSER:
        return "PAR";
    case Category::EXECUTOR:
        return "EXE";
    default:
        return "UNK";
    }
}

std::string DebugService::get_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) %
              1000;

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

void DebugService::start_timer(const std::string &name) {
    timers_[name] = std::chrono::steady_clock::now();
}

void DebugService::end_timer(const std::string &name) {
    auto it = timers_.find(name);
    if (it != timers_.end()) {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration<double>(end_time - it->second);
        stats_.timer_results[name] = duration.count();
        timers_.erase(it);

        // タイマー結果をログ出力
        debug(Category::GENERAL, "Timer '%s': %.6f seconds", name.c_str(),
              duration.count());
    }
}

void DebugService::reset_stats() {
    stats_ = DebugStats{};
    timers_.clear();
}

// ScopedDebug の実装
DebugService::ScopedDebug::ScopedDebug(Category category,
                                       const std::string &scope_name)
    : category_(category), scope_name_(scope_name),
      start_time_(std::chrono::steady_clock::now()) {
    DebugService::instance().debug(category_, "Entering scope: %s",
                                   scope_name_.c_str());
}

DebugService::ScopedDebug::~ScopedDebug() {
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration<double>(end_time - start_time_);
    DebugService::instance().debug(category_,
                                   "Exiting scope: %s (%.6f seconds)",
                                   scope_name_.c_str(), duration.count());
}
