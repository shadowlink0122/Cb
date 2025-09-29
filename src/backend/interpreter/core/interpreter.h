#pragma once
#include "../../../common/ast.h"
#include "../../../common/debug.h"
#include <iostream>
#include <map>
#include <string>
#include <vector>

// 前方宣言
class OutputManager;
class StatementExecutor;
class VariableManager;
class ArrayManager;
class TypeManager;
class ExpressionEvaluator;
class StatementExecutor;
class CommonOperations;
class VariableAccessService; // DRY効率化: 統一変数アクセスサービス
class ExpressionService;     // DRY効率化: 統一式評価サービス
class ArrayProcessingService; // DRY効率化: 統一配列処理サービス
class EnumManager;           // enum管理サービス
class CommonOperations;
class RecursiveParser;       // enum定義同期用

// 変数・関数の格納構造
struct Variable {
    TypeInfo type;
    bool is_const;
    bool is_array;
    bool is_assigned;
    bool is_multidimensional;     // 多次元配列フラグ
    bool is_struct;               // struct型かどうか
    std::string struct_type_name; // struct型名

    // union型用
    std::string type_name;        // union型名（union型の場合）
    TypeInfo current_type;        // union変数の現在の型

    // 値
    int64_t value;
    std::string str_value;

    // 配列用
    int array_size;
    std::vector<int64_t> array_values;
    std::vector<std::string> array_strings;

    // struct用メンバ変数
    std::map<std::string, Variable> struct_members;

    // interface用
    std::string interface_name;        // interface型の場合のinterface名
    std::string implementing_struct;   // interfaceを実装しているstruct名

    // 多次元配列用
    ArrayTypeInfo array_type_info;     // 多次元配列の型情報
    std::vector<int> array_dimensions; // 各次元のサイズ
    std::vector<int64_t>
        multidim_array_values; // 多次元配列データ（フラット化）
    std::vector<std::string> multidim_array_strings; // 多次元文字列配列データ

    // 将来の拡張計画 (struct/interface実装後):
    // - 静的配列: int[N] - 固定サイズ、コンパイル時型チェック
    // - 動的配列: int[] - 可変サイズ、実行時操作
    // - 共通メソッド: .size(), .len() - サイズ取得
    // - 動的配列専用メソッド: .push(value), .pop(), .clear() - 要素操作
    // - 境界チェック: 配列アクセス時の自動境界検証
    // - 型安全性: 異なるサイズの静的配列間での代入エラー検出

    Variable()
        : type(TYPE_INT), is_const(false), is_array(false), is_assigned(false),
          is_multidimensional(false), is_struct(false), current_type(TYPE_UNKNOWN), value(0),
          array_size(0) {
        // デバッグ用
        extern bool debug_mode;
        if (debug_mode) {
            debug_msg(DebugMsgId::VAR_CREATE_NEW);
        }
    }

    // 多次元配列用コンストラクタ
    Variable(const ArrayTypeInfo &array_info)
        : type(TYPE_INT), is_const(false), is_array(true), is_assigned(false),
          is_multidimensional(true), is_struct(false), current_type(TYPE_UNKNOWN), value(0), array_size(0),
          array_type_info(array_info) {}

    // struct用コンストラクタ
    Variable(const std::string &struct_name)
        : type(TYPE_STRUCT), is_const(false), is_array(false),
          is_assigned(false), is_multidimensional(false), is_struct(true),
          struct_type_name(struct_name), value(0), array_size(0) {
        // デバッグ用
        extern bool debug_mode;
        if (debug_mode) {
            debug_msg(DebugMsgId::VAR_MANAGER_STRUCT_CREATE, struct_name.c_str(), "struct");
        }
    }

    // interface用コンストラクタ  
    Variable(const std::string &interface_name, bool is_interface_flag)
        : type(TYPE_INTERFACE), is_const(false), is_array(false),
          is_assigned(false), is_multidimensional(false), is_struct(false),
          value(0), array_size(0), interface_name(interface_name) {
        // デバッグ用
        extern bool debug_mode;
        if (debug_mode) {
            debug_msg(DebugMsgId::VAR_MANAGER_STRUCT_CREATE, interface_name.c_str(), "interface");
        }
    }

    // 多次元配列のフラットインデックス計算
    int calculate_flat_index(const std::vector<int> &indices) const {
        debug_msg(DebugMsgId::FLAT_INDEX_CALCULATED, 
                  std::to_string(indices.size()).c_str(), 
                  std::to_string(array_type_info.dimensions.size()).c_str());
        if (indices.size() != array_type_info.dimensions.size()) {
            throw std::runtime_error("Dimension mismatch in array access");
        }

        int flat_index = 0;
        int multiplier = 1;

        // 最後の次元から計算（row-major order）
        for (int i = static_cast<int>(indices.size()) - 1; i >= 0; --i) {
            if (indices[i] < 0 ||
                indices[i] >= array_type_info.dimensions[i].size) {
                throw std::runtime_error("Array index out of bounds");
            }
            flat_index += indices[i] * multiplier;
            multiplier *= array_type_info.dimensions[i].size;
        }

        return flat_index;
    }

    // 多次元配列のサイズ検証
    bool validate_indices(const std::vector<int> &indices) const {
        if (indices.size() != array_type_info.dimensions.size()) {
            return false;
        }

        for (size_t i = 0; i < indices.size(); ++i) {
            if (indices[i] < 0 ||
                indices[i] >= array_type_info.dimensions[i].size) {
                return false;
            }
        }

        return true;
    }
};

// スコープ管理
struct Scope {
    std::map<std::string, Variable> variables;
    std::map<std::string, const ASTNode *> functions;

    void clear() {
        variables.clear();
        functions.clear();
    }
};
class ReturnException {
  public:
    int64_t value;
    std::string str_value;
    TypeInfo type;

    // 配列戻り値サポート
    bool is_array = false;
    std::vector<std::vector<std::vector<int64_t>>> int_array_3d;
    std::vector<std::vector<std::vector<std::string>>> str_array_3d;
    std::string array_type_name;
    
    // struct戻り値サポート
    bool is_struct = false;
    Variable struct_value;

    // 完全初期化コンストラクタ群
    ReturnException(int64_t val, TypeInfo t = TYPE_INT) 
        : value(val), str_value(""), type(t), is_array(false), is_struct(false) {}
    ReturnException(const std::string &str)
        : value(0), str_value(str), type(TYPE_STRING), is_array(false), is_struct(false) {}

    // 配列戻り値用コンストラクタ
    ReturnException(const std::vector<std::vector<std::vector<int64_t>>> &arr,
                    const std::string &type_name, TypeInfo t)
        : value(0), str_value(""), type(t), is_array(true), int_array_3d(arr),
          array_type_name(type_name), is_struct(false) {}

    ReturnException(
        const std::vector<std::vector<std::vector<std::string>>> &arr,
        const std::string &type_name, TypeInfo t)
        : value(0), str_value(""), type(t), is_array(true), str_array_3d(arr),
          array_type_name(type_name), is_struct(false) {}
    
    // struct戻り値用コンストラクタ  
    ReturnException(const Variable &struct_var)
        : value(0), str_value(""), type(TYPE_STRUCT), is_array(false), is_struct(true), struct_value(struct_var) {}
};

class BreakException {
  public:
    int64_t condition;
    BreakException(int64_t cond = 1) : condition(cond) {}
};

class ContinueException {
  public:
    int64_t condition;
    ContinueException(int64_t cond = 1) : condition(cond) {}
};

// インタープリター実装
class Interpreter : public EvaluatorInterface {
  private:
    std::vector<Scope> scope_stack;
    Scope global_scope;
    bool debug_mode;
    std::unique_ptr<OutputManager> output_manager_;
    std::map<std::string, std::string>
        typedef_map; // typedef alias -> base type mapping
    std::map<std::string, Variable> static_variables; // static変数の保存
    std::map<std::string, StructDefinition>
        struct_definitions_; // struct定義の保存
    std::map<std::string, InterfaceDefinition>
        interface_definitions_; // interface定義の保存
    std::vector<ImplDefinition>
        impl_definitions_; // impl定義の保存

    // Manager instances
    std::unique_ptr<VariableManager> variable_manager_;
    std::unique_ptr<ArrayManager> array_manager_;
    std::unique_ptr<TypeManager> type_manager_;
    std::unique_ptr<ExpressionEvaluator> expression_evaluator_;
    std::unique_ptr<StatementExecutor> statement_executor_;
    std::unique_ptr<CommonOperations> common_operations_;
    std::unique_ptr<VariableAccessService>
        variable_access_service_; // DRY効率化: 統一変数アクセス
    std::unique_ptr<ExpressionService>
        expression_service_; // DRY効率化: 統一式評価
    std::unique_ptr<ArrayProcessingService>
        array_processing_service_; // DRY効率化: 統一配列処理
    std::unique_ptr<EnumManager>
        enum_manager_; // enum管理サービス

    // Grant access to managers
    friend class VariableManager;
    friend class ArrayManager;
    friend class TypeManager;
    friend class ArrayProcessingService; // DRY効率化: 配列処理統合サービス
    friend class EnumManager;            // enum管理サービス

  public:
    Interpreter(bool debug = false);
    virtual ~Interpreter();

    // EvaluatorInterface実装
    void process(const ASTNode *ast) override;
    int64_t evaluate(const ASTNode *node) override;

    // 出力管理
    OutputManager &get_output_manager() { return *output_manager_; }

    // スコープ管理
    void push_scope();
    void pop_scope();
    void push_interpreter_scope() { push_scope(); }
    void pop_interpreter_scope() { pop_scope(); }
    Scope &current_scope();
    Scope &get_current_scope() { return current_scope(); }
    Scope &get_global_scope() { return global_scope; }

    // 変数・関数アクセス
    Variable *find_variable(const std::string &name);
    Variable *get_variable(const std::string &name) {
        return find_variable(name);
    }
    const ASTNode *find_function(const std::string &name);
    const ASTNode *get_function(const std::string &name) {
        return find_function(name);
    }

    // AST処理
    void register_global_declarations(const ASTNode *node);
    void initialize_global_variables(const ASTNode *node);
    void execute_statement(const ASTNode *node);
    void exec_statement(const ASTNode *node) { execute_statement(node); }
    int64_t eval_expression(const ASTNode *node) { return evaluate(node); }

    // 変数操作 (publicに移動)
    void assign_variable(const std::string &name, int64_t value,
                         TypeInfo type = TYPE_INT);
    void assign_variable(const std::string &name, const std::string &value);
    void assign_variable(const std::string &name, int64_t value, TypeInfo type,
                         bool is_const);
    void assign_variable(const std::string &name, const std::string &value,
                         bool is_const);
    void assign_function_parameter(const std::string &name, int64_t value,
                                   TypeInfo type);
    void assign_array_parameter(const std::string &name,
                                const Variable &source_array, TypeInfo type);
    void assign_array_element(const std::string &name, int64_t index,
                              int64_t value);
    void assign_string_element(const std::string &name, int64_t index,
                               const std::string &value);

    // 配列リテラル割り当て
    void assign_array_literal(const std::string &name,
                              const ASTNode *literal_node);

    // 関数戻り値からの配列割り当て
    void assign_array_from_return(const std::string &name,
                                  const ReturnException &ret);

    // 型解決
    TypeInfo resolve_type_alias(TypeInfo base_type,
                                const std::string &type_name);

    // typedef処理
    std::string resolve_typedef(const std::string &type_name);

    // struct処理
    void register_struct_definition(const std::string &struct_name,
                                    const StructDefinition &definition);
    const StructDefinition *
    find_struct_definition(const std::string &struct_name);
    void create_struct_variable(const std::string &var_name,
                                const std::string &struct_type_name);
    Variable *get_struct_member(const std::string &var_name,
                                const std::string &member_name);
    void assign_struct_literal(const std::string &var_name,
                               const ASTNode *literal_node);
    void assign_struct_member(const std::string &var_name,
                              const std::string &member_name, int64_t value);
    void assign_struct_member(const std::string &var_name,
                              const std::string &member_name,
                              const std::string &value);
    void assign_struct_member_array_element(const std::string &var_name,
                                            const std::string &member_name,
                                            int index, int64_t value);
    void assign_struct_member_array_element(const std::string &var_name,
                                            const std::string &member_name,
                                            int index,
                                            const std::string &value);
    int64_t get_struct_member_array_element(const std::string &var_name,
                                            const std::string &member_name,
                                            int index);
    // N次元配列アクセス対応版
    int64_t get_struct_member_multidim_array_element(const std::string &var_name,
                                                    const std::string &member_name,
                                                    const std::vector<int64_t> &indices);
    std::string get_struct_member_array_string_element(const std::string &var_name,
                                                      const std::string &member_name,
                                                      int index);
    void assign_struct_member_array_literal(const std::string &var_name,
                                            const std::string &member_name,
                                            const ASTNode *array_literal);
    TypeInfo string_to_type_info(const std::string &type_str);
    void sync_struct_members_from_direct_access(const std::string &var_name);

    // static変数処理
    Variable *find_static_variable(const std::string &name);
    void create_static_variable(const std::string &name, const ASTNode *node);

    // interface管理
    void register_interface_definition(const std::string &interface_name,
                                      const InterfaceDefinition &definition);
    const InterfaceDefinition *
    find_interface_definition(const std::string &interface_name);
    
    // impl管理
    void register_impl_definition(const ImplDefinition &impl_def);
    const ImplDefinition *find_impl_for_struct(const std::string &struct_name, 
                                               const std::string &interface_name);
    
    // interface型変数管理
    void create_interface_variable(const std::string &var_name, 
                                  const std::string &interface_name);
    Variable *get_interface_variable(const std::string &var_name);
    
    // impl定義へのアクセサ
    const std::vector<ImplDefinition>& get_impl_definitions() const {
        return impl_definitions_;
    }

    // 関数コンテキスト
    std::string current_function_name; // 現在実行中の関数名

  private:
    void print_value(const ASTNode *expr);
    void print_formatted(const ASTNode *format_str, const ASTNode *arg_list);

    // N次元配列リテラル処理の再帰関数
    void process_ndim_array_literal(const ASTNode *literal_node, Variable &var,
                                    TypeInfo elem_type, int &flat_index,
                                    int max_size);

  public:
    void check_type_range(TypeInfo type, int64_t value,
                          const std::string &name);

    // エラー表示ヘルパー関数
    void throw_runtime_error_with_location(const std::string &message,
                                           const ASTNode *node = nullptr);
    void print_error_at_node(const std::string &message,
                             const ASTNode *node = nullptr);

    // デバッグ機能
    void set_debug_mode(bool debug) { debug_mode = debug; }
    bool is_debug_mode() const { return debug_mode; }

    // 共通操作へのアクセス
    CommonOperations *get_common_operations() {
        return common_operations_.get();
    }

    // ExpressionEvaluatorへのアクセス
    ExpressionEvaluator *get_expression_evaluator() {
        return expression_evaluator_.get();
    }

    // TypeManagerへのアクセス
    TypeManager *get_type_manager() { return type_manager_.get(); }
    
    // EnumManagerへのアクセス
    EnumManager *get_enum_manager() { return enum_manager_.get(); }
    
    // VariableManagerへのアクセス
    VariableManager *get_variable_manager() { return variable_manager_.get(); }
    
    // 関数定義の検索
    const ASTNode* find_function_definition(const std::string& func_name);
    
    // Parserからのenum定義同期
    void sync_enum_definitions_from_parser(RecursiveParser* parser);
    
    // Parserからのstruct定義同期
    void sync_struct_definitions_from_parser(RecursiveParser* parser);

    // N次元配列アクセス用のヘルパー関数
    std::string extract_array_name(const ASTNode *node);
    std::vector<int64_t> extract_array_indices(const ASTNode *node);
    std::string extract_array_element_name(const ASTNode *node);
    
    // Priority 3: 変数ポインターから名前を取得するヘルパー
    std::string find_variable_name(const Variable* var);

    // ArrayManagerへのアクセス
    ArrayManager *get_array_manager() { return array_manager_.get(); }

    // DRY効率化: サービスクラスへのアクセサ
    ExpressionService *get_expression_service() {
        return expression_service_.get();
    }
    VariableAccessService *get_variable_access_service() {
        return variable_access_service_.get();
    }
    ArrayProcessingService *get_array_processing_service() {
        return array_processing_service_.get();
    }

    int64_t
    getMultidimensionalArrayElement(const Variable &var,
                                    const std::vector<int64_t> &indices);

    // Priority 3: 統一された配列要素アクセス (ArrayProcessingService経由)
    int64_t
    getMultidimensionalArrayElement(Variable &var,
                                    const std::vector<int64_t> &indices);
    void setMultidimensionalArrayElement(Variable &var,
                                         const std::vector<int64_t> &indices,
                                         int64_t value);

    // Priority 3: 統一された文字列配列アクセス (ArrayProcessingService経由)
    std::string
    getMultidimensionalStringArrayElement(Variable &var,
                                          const std::vector<int64_t> &indices);
    void
    setMultidimensionalStringArrayElement(Variable &var,
                                          const std::vector<int64_t> &indices,
                                          const std::string &value);
                                          
    // self処理用ヘルパー関数
    std::string get_self_receiver_path();
    void sync_self_to_receiver(const std::string& receiver_path);
};
