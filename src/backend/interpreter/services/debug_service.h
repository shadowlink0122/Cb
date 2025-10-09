#pragma once

#include <chrono>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

/**
 * 統一デバッグサービス - DRY原則に基づくデバッグ出力の統合
 * 全てのクラスが共通して使用するデバッグ機能を提供
 */
class DebugService {
  public:
    // デバッグレベル定義
    enum class Level {
        TRACE = 0,       // 最も詳細
        DEBUG_LEVEL = 1, // デバッグ情報
        INFO = 2,        // 一般情報
        WARN = 3,        // 警告
        ERROR = 4        // エラーのみ
    };

    // カテゴリ定義（機能別デバッグ制御）
    enum class Category {
        GENERAL,
        EXPRESSION,
        VARIABLE,
        ARRAY,
        STRUCT,
        FUNCTION,
        PARSER,
        EXECUTOR
    };

    static DebugService &instance();

    /**
     * デバッグ設定
     */
    void set_level(Level level) { current_level_ = level; }
    void set_enabled(bool enabled) { debug_enabled_ = enabled; }
    void set_category_enabled(Category category, bool enabled);
    void set_output_file(const std::string &filename);
    void set_timestamp_enabled(bool enabled) { timestamp_enabled_ = enabled; }

    /**
     * 統一デバッグ出力（フォーマット指定）
     */
    template <typename... Args>
    void log(Level level, Category category, const std::string &format,
             Args... args) {
        if (!should_log(level, category))
            return;

        std::string message = format_message(format, args...);
        output_message(level, category, message);
        update_stats(category);
    }

    /**
     * 簡単なデバッグ出力メソッド
     */
    template <typename... Args>
    void trace(Category category, const std::string &format, Args... args) {
        log(Level::TRACE, category, format, args...);
    }

    template <typename... Args>
    void debug(Category category, const std::string &format, Args... args) {
        log(Level::DEBUG_LEVEL, category, format, args...);
    }

    template <typename... Args>
    void info(Category category, const std::string &format, Args... args) {
        log(Level::INFO, category, format, args...);
    }

    template <typename... Args>
    void warn(Category category, const std::string &format, Args... args) {
        log(Level::WARN, category, format, args...);
    }

    template <typename... Args>
    void error(Category category, const std::string &format, Args... args) {
        log(Level::ERROR, category, format, args...);
    }

    /**
     * スコープ付きデバッグ（RAII）
     */
    class ScopedDebug {
      public:
        ScopedDebug(Category category, const std::string &scope_name);
        ~ScopedDebug();

      private:
        Category category_;
        std::string scope_name_;
        std::chrono::steady_clock::time_point start_time_;
    };

    /**
     * パフォーマンス測定
     */
    void start_timer(const std::string &name);
    void end_timer(const std::string &name);

    /**
     * デバッグ統計の取得
     */
    struct DebugStats {
        std::unordered_map<Category, size_t> message_counts;
        size_t total_messages = 0;
        std::unordered_map<std::string, double> timer_results;
    };

    const DebugStats &get_stats() const { return stats_; }
    void reset_stats();

  private:
    DebugService() = default;

    Level current_level_ = Level::INFO;
    bool debug_enabled_ = false;
    bool timestamp_enabled_ = true;
    std::unordered_map<Category, bool> category_enabled_;
    std::unique_ptr<std::ofstream> log_file_;
    DebugStats stats_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point>
        timers_;

    bool should_log(Level level, Category category) const;
    void output_message(Level level, Category category,
                        const std::string &message);
    void update_stats(Category category);

    std::string level_to_string(Level level) const;
    std::string category_to_string(Category category) const;
    std::string get_timestamp() const;

    template <typename... Args>
    std::string format_message(const std::string &format, Args... args) {
        if (sizeof...(args) == 0) {
            return format;
        }

        // format stringの警告を抑制するため、"%s"を使用
        size_t size = std::snprintf(nullptr, 0, "%s", format.c_str()) + 1;
        if (sizeof...(args) > 0) {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
#endif
            size = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
        }
        std::unique_ptr<char[]> buf(new char[size]);
        if (sizeof...(args) > 0) {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
#endif
            std::snprintf(buf.get(), size, format.c_str(), args...);
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
        } else {
            std::snprintf(buf.get(), size, "%s", format.c_str());
        }
        return std::string(buf.get(), buf.get() + size - 1);
    }
};

// 便利なマクロ定義
#define DEBUG_TRACE(category, format, ...)                                     \
    DebugService::instance().trace(DebugService::Category::category, format,   \
                                   ##__VA_ARGS__)

#define DEBUG_DEBUG(category, format, ...)                                     \
    DebugService::instance().debug(DebugService::Category::category, format,   \
                                   ##__VA_ARGS__)

#define DEBUG_INFO(category, format, ...)                                      \
    DebugService::instance().info(DebugService::Category::category, format,    \
                                  ##__VA_ARGS__)

#define DEBUG_WARN(category, format, ...)                                      \
    DebugService::instance().warn(DebugService::Category::category, format,    \
                                  ##__VA_ARGS__)

#define DEBUG_ERROR(category, format, ...)                                     \
    DebugService::instance().error(DebugService::Category::category, format,   \
                                   ##__VA_ARGS__)

#define DEBUG_SCOPE(category, name)                                            \
    DebugService::ScopedDebug _debug_scope(DebugService::Category::category,   \
                                           name)

#define DEBUG_TIMER_START(name) DebugService::instance().start_timer(name)

#define DEBUG_TIMER_END(name) DebugService::instance().end_timer(name)
