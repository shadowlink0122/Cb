#pragma once
#include <map>
#include <string>
#include <vector>

// CB言語の設定を管理するクラス
class CbConfig {
  public:
    struct PathConfig {
        std::string stdlib;
        std::string modules;
        std::string user_modules;
    };

    struct CompilerConfig {
        std::string default_target;
        std::vector<std::string> supported_targets;
    };

    struct DebugConfig {
        bool enable_module_debug;
        bool verbose_import;
    };

  private:
    PathConfig paths_;
    CompilerConfig compiler_;
    DebugConfig debug_;
    std::vector<std::string> search_order_;
    std::string config_file_path_;

  public:
    CbConfig();
    ~CbConfig() = default;

    // 設定ファイルの読み込み
    bool load_config(const std::string &config_path = "");
    bool load_from_json(const std::string &json_content);

    // パス解決
    std::vector<std::string> get_module_search_paths() const;
    std::string resolve_path(const std::string &path) const;

    // アクセサ
    const PathConfig &get_paths() const { return paths_; }
    const CompilerConfig &get_compiler() const { return compiler_; }
    const DebugConfig &get_debug() const { return debug_; }
    const std::vector<std::string> &get_search_order() const {
        return search_order_;
    }

    // デフォルト設定の生成
    void create_default_config(const std::string &output_path);

  private:
    std::string expand_path(const std::string &path) const;
    std::string get_home_directory() const;
    std::string get_executable_directory() const;
};
