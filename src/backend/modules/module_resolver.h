#pragma once
#include "../../common/ast.h"
#include "../../common/cb_config.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

// モジュール情報を管理する構造体
struct ModuleInfo {
    std::string name;
    std::string file_path;
    std::unique_ptr<ASTNode> ast;
    std::vector<std::string> exported_functions;
    std::vector<std::string> exported_variables;
    bool is_loaded = false;
};

// モジュール解決・管理システム
class ModuleResolver {
  public:
    ModuleResolver();
    ModuleResolver(const CbConfig& config);
    ~ModuleResolver() = default;
    
    // 設定の更新
    void update_config(const CbConfig& config);
    
    // モジュール解決
    bool resolve_import(const std::string &module_name);
    ModuleInfo *get_module(const std::string &module_name);
    
    // モジュールパスの管理
    void add_module_path(const std::string &path);
    std::vector<std::string> get_module_paths() const;
    
    // 組み込みモジュールの登録
    void register_builtin_modules();
    
    // モジュール関数呼び出しサポート
    bool is_module_loaded(const std::string& module_name);
    const ASTNode* find_module_function(const std::string& module_name, const std::string& function_name);
    int64_t find_module_variable(const std::string& module_name, const std::string& variable_name);
    
  private:
    std::map<std::string, std::unique_ptr<ModuleInfo>> modules_;
    std::vector<std::string> module_paths_;
    std::unique_ptr<CbConfig> config_;
    
    // ヘルパーメソッド
    std::string get_executable_directory();
    void initialize_paths();
    
    // モジュールファイルの検索・読み込み
    std::string find_module_file(const std::string &module_name);
    bool load_module_file(const std::string &module_name, const std::string &file_path);
    
    // AST内の関数検索
    const ASTNode* find_function_in_ast(const ASTNode* node, const std::string& function_name);
    
    // AST内の変数検索
    int64_t find_variable_in_ast(const ASTNode* node, const std::string& variable_name);
    
    // 組み込みモジュール作成
    void create_std_io_module();
};
