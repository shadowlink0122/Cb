#include "cb_config.h"
#include <fstream>
#include <iostream>
#include <pwd.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

CbConfig::CbConfig() {
    // デフォルト値を設定
    paths_.stdlib = "./stdlib";
    paths_.modules = "./lib";
    paths_.user_modules = "~/.cb/modules";

    compiler_.default_target = "native";
    compiler_.supported_targets = {"native", "baremetal", "wasm"};

    debug_.enable_module_debug = false;
    debug_.verbose_import = false;

    search_order_ = {"user_modules", "modules", "stdlib", "current_directory"};
}

bool CbConfig::load_config(const std::string &config_path) {
    std::string config_file = config_path;

    if (config_file.empty()) {
        // デフォルトの設定ファイルを検索
        std::vector<std::string> default_paths = {
            "./cb_config.json", get_executable_directory() + "cb_config.json",
            "../../cb_config.json",    // 単体テスト用 (tests/unit
                                       // から見た相対パス)
            "../../../cb_config.json", // 統合テスト用 (tests/integration
                                       // から見た相対パス)
            "~/.cb/config.json"};

        for (const auto &path : default_paths) {
            std::string expanded = expand_path(path);
            // std::cout << "[DEBUG] Checking config path: " << expanded <<
            // std::endl;
            std::ifstream file(expanded);
            if (file.good()) {
                config_file = expanded;
                // std::cout << "Found config file at: " << config_file <<
                // std::endl;
                break;
            }
        }
    }

    if (config_file.empty()) {
        // デバッグモードまたは明示的にverboseが有効な場合のみメッセージを出力
        // 単体テストやサイレント実行時は出力しない
        char *env_debug = std::getenv("CB_DEBUG_MODE");
        char *env_verbose = std::getenv("CB_VERBOSE");
        if ((env_debug && env_debug[0] == '1') ||
            (env_verbose && env_verbose[0] == '1')) {
            std::cout << "No configuration file found, using defaults"
                      << std::endl;
        }
        return true; // デフォルト設定を使用
    }

    std::ifstream file(config_file);
    if (!file.is_open()) {
        std::cerr << "Cannot open config file: " << config_file << std::endl;
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    config_file_path_ = config_file;
    bool result = load_from_json(buffer.str());
    // std::cout << "Loaded configuration from: " << config_file_path_ <<
    // std::endl;
    return result;
}

bool CbConfig::load_from_json(const std::string &json_content) {
    // シンプルなJSONパーサー（基本的なkey:valueのみ対応）
    std::istringstream stream(json_content);
    std::string line;

    while (std::getline(stream, line)) {
        // 簡単なkey:value解析
        size_t colon = line.find(':');
        if (colon == std::string::npos)
            continue;

        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);

        // 空白と引用符を削除
        auto trim = [](std::string &s) {
            size_t start = s.find_first_not_of(" \t\n\r\"");
            if (start == std::string::npos) {
                s = "";
                return;
            }
            size_t end = s.find_last_not_of(" \t\n\r\",\"");
            s = s.substr(start, end - start + 1);
        };

        trim(key);
        trim(value);

        // 設定値を適用
        if (key == "\"stdlib\"") {
            paths_.stdlib = expand_path(value);
        } else if (key == "\"modules\"") {
            paths_.modules = expand_path(value);
        } else if (key == "\"user_modules\"") {
            paths_.user_modules = expand_path(value);
        } else if (key == "\"default_target\"") {
            compiler_.default_target = value;
        } else if (key == "\"enable_module_debug\"") {
            debug_.enable_module_debug = (value == "true");
        } else if (key == "\"verbose_import\"") {
            debug_.verbose_import = (value == "true");
        }
    }

    // std::cout << "Loaded configuration from: " << config_file_path_ <<
    // std::endl;
    return true;
}

std::vector<std::string> CbConfig::get_module_search_paths() const {
    std::vector<std::string> paths;

    for (const auto &order : search_order_) {
        if (order == "user_modules") {
            paths.push_back(paths_.user_modules);
        } else if (order == "modules") {
            paths.push_back(paths_.modules);
        } else if (order == "stdlib") {
            paths.push_back(paths_.stdlib);
        } else if (order == "current_directory") {
            paths.push_back("./");
        }
    }

    return paths;
}

std::string CbConfig::resolve_path(const std::string &path) const {
    return expand_path(path);
}

std::string CbConfig::expand_path(const std::string &path) const {
    if (path.empty())
        return path;

    if (path[0] == '~') {
        return get_home_directory() + path.substr(1);
    }

    // 相対パスの場合、実行ファイルのディレクトリを基準にする
    if (path.substr(0, 2) == "./") {
        return get_executable_directory() + path.substr(2);
    }

    return path;
}

std::string CbConfig::get_home_directory() const {
    const char *home = getenv("HOME");
    if (home)
        return std::string(home);

    struct passwd *pwd = getpwuid(getuid());
    if (pwd)
        return std::string(pwd->pw_dir);

    return "/tmp";
}

std::string CbConfig::get_executable_directory() const {
    char result[PATH_MAX];
    std::string executable_path;

#ifdef __APPLE__
    uint32_t size = sizeof(result);
    if (_NSGetExecutablePath(result, &size) == 0) {
        executable_path = std::string(result);
    } else {
        return "./";
    }
#else
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count != -1) {
        executable_path = std::string(result, count);
    } else {
        return "./";
    }
#endif

    size_t last_slash = executable_path.find_last_of('/');
    if (last_slash != std::string::npos) {
        return executable_path.substr(0, last_slash + 1);
    }

    return "./";
}

void CbConfig::create_default_config(const std::string &output_path) {
    std::ofstream file(output_path);
    if (!file.is_open()) {
        std::cerr << "Cannot create config file: " << output_path << std::endl;
        return;
    }

    file << R"({
  "version": "1.0",
  "language": {
    "name": "Cb",
    "version": "0.1.0"
  },
  "paths": {
    "stdlib": ")"
         << get_executable_directory() << R"(stdlib",
    "modules": ")"
         << get_executable_directory() << R"(lib",
    "user_modules": "~/.cb/modules"
  },
  "search_order": [
    "user_modules",
    "modules", 
    "stdlib",
    "current_directory"
  ],
  "compiler": {
    "default_target": "native",
    "supported_targets": ["native", "baremetal", "wasm"]
  },
  "debug": {
    "enable_module_debug": false,
    "verbose_import": false
  }
})";

    file.close();
    std::cout << "Created default configuration file: " << output_path
              << std::endl;
}
