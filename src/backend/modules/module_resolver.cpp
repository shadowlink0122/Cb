#include "module_resolver.h"
#include "../../frontend/parser_utils.h"
#include "../../common/ast.h"
#include "../../common/cb_config.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <unistd.h>
#include <limits.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

extern int yyparse();
extern FILE *yyin;
extern std::unique_ptr<ASTNode> root_node;
extern int yylineno;

// 実行ファイルのディレクトリを取得するヘルパー関数
std::string ModuleResolver::get_executable_directory() {
    char result[PATH_MAX];
    std::string executable_path;
    
#ifdef __APPLE__
    // macOSの場合
    uint32_t size = sizeof(result);
    if (_NSGetExecutablePath(result, &size) == 0) {
        executable_path = std::string(result);
    } else {
        // フォールバック: カレントディレクトリ
        return "./";
    }
#else
    // Linuxの場合
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count != -1) {
        executable_path = std::string(result, count);
    } else {
        return "./";
    }
#endif
    
    // ディレクトリ部分を抽出
    size_t last_slash = executable_path.find_last_of('/');
    if (last_slash != std::string::npos) {
        return executable_path.substr(0, last_slash + 1);
    }
    
    return "./";
}

ModuleResolver::ModuleResolver() {
    config_ = std::make_unique<CbConfig>();
    config_->load_config();
    initialize_paths();
    register_builtin_modules();
}

ModuleResolver::ModuleResolver(const CbConfig& config) {
    config_ = std::make_unique<CbConfig>(config);
    initialize_paths();
    register_builtin_modules();
}

void ModuleResolver::update_config(const CbConfig& config) {
    config_ = std::make_unique<CbConfig>(config);
    module_paths_.clear();
    initialize_paths();
}

void ModuleResolver::initialize_paths() {
    // 設定ファイルからモジュールパスを取得
    module_paths_ = config_->get_module_search_paths();
    
    if (config_->get_debug().verbose_import) {
        std::cout << "Module search paths:" << std::endl;
        for (const auto& path : module_paths_) {
            std::cout << "  - " << path << std::endl;
        }
    }
}

bool ModuleResolver::resolve_import(const std::string &module_name) {
    // 既にロードされている場合はスキップ
    if (modules_.find(module_name) != modules_.end() && 
        modules_[module_name]->is_loaded) {
        return true;
    }
    
    // 組み込みモジュールかチェック
    if (module_name == "stdio") {
        // stdioは既に登録済み
        return true;
    }
    
    // モジュールファイルを検索
    std::string file_path = find_module_file(module_name);
    if (file_path.empty()) {
        std::cerr << "Module not found: " << module_name << std::endl;
        std::cerr << "Searched in paths:" << std::endl;
        for (const auto& path : module_paths_) {
            std::cerr << "  - " << path << std::endl;
        }
        return false;
    }
    
    // モジュールファイルを読み込み・パース
    return load_module_file(module_name, file_path);
}

ModuleInfo *ModuleResolver::get_module(const std::string &module_name) {
    auto it = modules_.find(module_name);
    if (it != modules_.end()) {
        return it->second.get();
    }
    return nullptr;
}

void ModuleResolver::add_module_path(const std::string &path) {
    module_paths_.push_back(path);
}

std::vector<std::string> ModuleResolver::get_module_paths() const {
    return module_paths_;
}

std::string ModuleResolver::find_module_file(const std::string &module_name) {
    if (config_->get_debug().verbose_import) {
        std::cout << "[DEBUG] Searching for module: " << module_name << std::endl;
    }
    
    // 各モジュールパスでファイルを検索
    for (const auto &path : module_paths_) {
        if (config_->get_debug().verbose_import) {
            std::cout << "[DEBUG] Checking path: " << path << std::endl;
        }
        
        // 1. まずそのままのファイル名で検索 (例: stdio.cb)
        std::string direct_path = path + "/" + module_name + ".cb";
        if (config_->get_debug().verbose_import) {
            std::cout << "[DEBUG] Trying: " << direct_path << std::endl;
        }
        std::ifstream direct_file(direct_path);
        if (direct_file.good()) {
            if (config_->get_debug().verbose_import) {
                std::cout << "[DEBUG] Found module file: " << direct_path << std::endl;
            }
            return direct_path;
        }
        
        // 2. ドット記法を階層パスに変換 (例: std.io -> std/io.cb)
        std::string hierarchical_path = module_name;
        std::replace(hierarchical_path.begin(), hierarchical_path.end(), '.', '/');
        std::string full_hierarchical_path = path + "/" + hierarchical_path + ".cb";
        if (config_->get_debug().verbose_import) {
            std::cout << "[DEBUG] Trying hierarchical: " << full_hierarchical_path << std::endl;
        }
        std::ifstream hierarchical_file(full_hierarchical_path);
        if (hierarchical_file.good()) {
            if (config_->get_debug().verbose_import) {
                std::cout << "[DEBUG] Found hierarchical module file: " << full_hierarchical_path << std::endl;
            }
            return full_hierarchical_path;
        }
        
        // 3. 階層ディレクトリ内のindex.cb (例: std.io -> std/io/index.cb)
        std::string index_path = path + "/" + hierarchical_path + "/index.cb";
        if (config_->get_debug().verbose_import) {
            std::cout << "[DEBUG] Trying index: " << index_path << std::endl;
        }
        std::ifstream index_file(index_path);
        if (index_file.good()) {
            if (config_->get_debug().verbose_import) {
                std::cout << "[DEBUG] Found index module file: " << index_path << std::endl;
            }
            return index_path;
        }
    }
    
    return ""; // 見つからない場合は空文字列を返す
}

bool ModuleResolver::load_module_file(const std::string &module_name, 
                                     const std::string &file_path) {
    // ファイルを開く
    FILE *file = std::fopen(file_path.c_str(), "r");
    if (!file) {
        std::cerr << "Cannot open module file: " << file_path << std::endl;
        return false;
    }
    
    // パーサー状態を完全に保存
    FILE *old_yyin = yyin;
    int old_yylineno = yylineno;
    std::unique_ptr<ASTNode> old_root = std::move(root_node);
    
    // パーサー状態をクリア
    yyin = file;
    yylineno = 1;
    root_node = nullptr;
    
    try {
        // モジュールファイルをパース
        int parse_result = yyparse();
        std::fclose(file);
        
        if (parse_result != 0 || !root_node) {
            std::cerr << "Failed to parse module: " << file_path << std::endl;
            std::cerr << "Parse result: " << parse_result << ", AST: " 
                      << (root_node ? "exists" : "null") << std::endl;
            
            // パーサー状態を復元
            yyin = old_yyin;
            yylineno = old_yylineno;
            root_node = std::move(old_root);
            return false;
        }
        
        // モジュール情報を作成
        auto module_info = std::make_unique<ModuleInfo>();
        module_info->name = module_name;
        module_info->file_path = file_path;
        module_info->ast = std::move(root_node);
        module_info->is_loaded = true;
        
        modules_[module_name] = std::move(module_info);
        
        if (config_->get_debug().verbose_import) {
            std::cout << "Successfully loaded module: " << module_name 
                      << " from " << file_path << std::endl;
        }
        
        // パーサー状態を復元
        yyin = old_yyin;
        yylineno = old_yylineno;
        root_node = std::move(old_root);
        
        return true;
        
    } catch (const std::exception &e) {
        if (file) std::fclose(file);
        std::cerr << "Exception loading module " << module_name << ": " << e.what() << std::endl;
        
        // パーサー状態を復元
        yyin = old_yyin;
        yylineno = old_yylineno;
        root_node = std::move(old_root);
        
        return false;
    }
}

void ModuleResolver::register_builtin_modules() {
    create_std_io_module();
}

void ModuleResolver::create_std_io_module() {
    // stdio モジュール (組み込み・後方互換用)
    auto stdio_module = std::make_unique<ModuleInfo>();
    stdio_module->name = "stdio";
    stdio_module->file_path = "<builtin>";
    stdio_module->is_loaded = true;
    
    // エクスポート関数を登録
    stdio_module->exported_functions.push_back("print");
    stdio_module->exported_functions.push_back("println");
    stdio_module->exported_functions.push_back("print_int");
    stdio_module->exported_functions.push_back("print_float");
    
    modules_["stdio"] = std::move(stdio_module);
}

bool ModuleResolver::is_module_loaded(const std::string& module_name) {
    auto it = modules_.find(module_name);
    return it != modules_.end() && it->second->is_loaded;
}

const ASTNode* ModuleResolver::find_module_function(const std::string& module_name, const std::string& function_name) {
    auto it = modules_.find(module_name);
    if (it == modules_.end() || !it->second->is_loaded) {
        return nullptr;
    }
    
    // モジュールのASTから関数を検索
    const ModuleInfo* module = it->second.get();
    if (!module->ast) {
        return nullptr;
    }
    
    return find_function_in_ast(module->ast.get(), function_name);
}

const ASTNode* ModuleResolver::find_function_in_ast(const ASTNode* node, const std::string& function_name) {
    if (!node) {
        return nullptr;
    }
    
    // 関数宣言ノードの場合
    if (node->node_type == ASTNodeType::AST_FUNC_DECL && node->name == function_name) {
        return node;
    }
    
    // 文リストの場合は子ノードを再帰的に検索
    if (node->node_type == ASTNodeType::AST_STMT_LIST) {
        for (const auto& stmt : node->statements) {
            const ASTNode* result = find_function_in_ast(stmt.get(), function_name);
            if (result) {
                return result;
            }
        }
    }
    
    return nullptr;
}

int64_t ModuleResolver::find_module_variable(const std::string& module_name, const std::string& variable_name) {
    auto it = modules_.find(module_name);
    if (it == modules_.end() || !it->second->is_loaded) {
        throw std::runtime_error("Module not loaded: " + module_name);
    }
    
    // モジュールのASTから変数を検索
    const ModuleInfo* module = it->second.get();
    if (!module->ast) {
        throw std::runtime_error("Module AST not available: " + module_name);
    }
    
    return find_variable_in_ast(module->ast.get(), variable_name);
}

int64_t ModuleResolver::find_variable_in_ast(const ASTNode* node, const std::string& variable_name) {
    if (!node) {
        throw std::runtime_error("Variable not found: " + variable_name);
    }
    
    // 変数宣言ノードの場合
    if (node->node_type == ASTNodeType::AST_VAR_DECL && node->name == variable_name) {
        return node->int_value;
    }
    
    // 文リストの場合は子ノードを再帰的に検索
    if (node->node_type == ASTNodeType::AST_STMT_LIST) {
        for (const auto& stmt : node->statements) {
            try {
                return find_variable_in_ast(stmt.get(), variable_name);
            } catch (const std::runtime_error&) {
                // 見つからない場合は次を試す
                continue;
            }
        }
    }
    
    throw std::runtime_error("Variable not found: " + variable_name);
}
