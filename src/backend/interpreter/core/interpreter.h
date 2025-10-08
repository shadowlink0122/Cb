#pragma once
#include "../../../common/ast.h"
#include "../../../common/debug.h"
#include "type_inference.h"
#include <cstdio>
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
class EnumManager;            // enum管理サービス
class StaticVariableManager;  // static変数管理サービス
class InterfaceOperations;    // interface/impl管理サービス
class StructOperations;       // struct操作管理サービス
class StructVariableManager;  // struct変数管理サービス
class CommonOperations;
class ControlFlowExecutor;        // 制御フロー実行サービス
class StatementListExecutor;      // 文リスト・複合文実行サービス
class ReturnHandler;              // return文処理サービス
class AssertionHandler;           // アサーション文処理サービス
class BreakContinueHandler;       // break/continue文処理サービス
class FunctionDeclarationHandler; // 関数宣言処理サービス
class StructDeclarationHandler;   // 構造体宣言処理サービス
class InterfaceDeclarationHandler; // インターフェース宣言処理サービス
class ImplDeclarationHandler;     // impl宣言処理サービス
class ExpressionStatementHandler; // 式文処理サービス
class RecursiveParser;            // enum定義同期用

// 変数・関数の格納構造
struct Variable {
    TypeInfo type;
    bool is_const;
    bool is_array;
    bool is_assigned;
    bool is_multidimensional;           // 多次元配列フラグ
    bool is_struct;                     // struct型かどうか
    bool is_pointer;                    // ポインタ型かどうか
    int pointer_depth;                  // ポインタの深さ
    std::string pointer_base_type_name; // ポインタ基底型名
    TypeInfo pointer_base_type;         // ポインタ基底型
    bool is_reference;                  // 参照型かどうか
    bool is_unsigned;                   // unsigned修飾子かどうか
    std::string struct_type_name;       // struct型名
    bool is_private_member;             // struct privateメンバーフラグ

    // union型用
    std::string type_name; // union型名（union型の場合）
    TypeInfo current_type; // union変数の現在の型

    // 値
    int64_t value;
    std::string str_value;
    float float_value;
    double double_value;
    long double quad_value;
    __int128_t big_value;

    // 配列用
    int array_size;
    std::vector<int64_t> array_values;
    std::vector<float> array_float_values;
    std::vector<double> array_double_values;
    std::vector<long double> array_quad_values;
    std::vector<std::string> array_strings;

    // struct用メンバ変数
    std::map<std::string, Variable> struct_members;

    // interface用
    std::string interface_name;      // interface型の場合のinterface名
    std::string implementing_struct; // interfaceを実装しているstruct名

    // 関数ポインタ用
    bool is_function_pointer;          // 関数ポインタかどうか
    std::string function_pointer_name; // 指している関数名

    // 多次元配列用
    ArrayTypeInfo array_type_info;     // 多次元配列の型情報
    std::vector<int> array_dimensions; // 各次元のサイズ
    std::vector<int64_t>
        multidim_array_values; // 多次元配列データ（フラット化、整数系）
    std::vector<float> multidim_array_float_values;
    std::vector<double> multidim_array_double_values;
    std::vector<long double> multidim_array_quad_values;
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
          is_multidimensional(false), is_struct(false), is_pointer(false),
          pointer_depth(0), pointer_base_type_name(""),
          pointer_base_type(TYPE_UNKNOWN), is_reference(false),
          is_unsigned(false), struct_type_name(""), is_private_member(false),
          type_name(""), current_type(TYPE_UNKNOWN), value(0), str_value(""),
          float_value(0.0f), double_value(0.0), quad_value(0.0L), big_value(0),
          array_size(0), is_function_pointer(false), function_pointer_name("") {
        // デバッグ出力を削除（無限再帰を防ぐため）
        // extern bool debug_mode;
        // if (debug_mode) {
        //     debug_msg(DebugMsgId::VAR_CREATE_NEW);
        // }
    }

    // 多次元配列用コンストラクタ
    Variable(const ArrayTypeInfo &array_info)
        : type(TYPE_INT), is_const(false), is_array(true), is_assigned(false),
          is_multidimensional(true), is_struct(false), is_pointer(false),
          pointer_depth(0), pointer_base_type_name(""),
          pointer_base_type(TYPE_UNKNOWN), is_reference(false),
          is_unsigned(false), struct_type_name(""), is_private_member(false),
          type_name(""), current_type(TYPE_UNKNOWN), value(0), str_value(""),
          float_value(0.0f), double_value(0.0), quad_value(0.0L), big_value(0),
          array_size(0), is_function_pointer(false), function_pointer_name(""),
          array_type_info(array_info) {}

    // struct用コンストラクタ
    Variable(const std::string &struct_name)
        : type(TYPE_STRUCT), is_const(false), is_array(false),
          is_assigned(false), is_multidimensional(false), is_struct(true),
          is_pointer(false), pointer_depth(0), pointer_base_type_name(""),
          pointer_base_type(TYPE_UNKNOWN), is_reference(false),
          is_unsigned(false), struct_type_name(struct_name),
          is_private_member(false), type_name(""), current_type(TYPE_UNKNOWN),
          value(0), str_value(""), float_value(0.0f), double_value(0.0),
          quad_value(0.0L), big_value(0), array_size(0),
          is_function_pointer(false), function_pointer_name("") {
        // デバッグ出力を削除（無限再帰を防ぐため）
        // extern bool debug_mode;
        // if (debug_mode) {
        //     debug_msg(DebugMsgId::VAR_MANAGER_STRUCT_CREATE,
        //     struct_name.c_str(), "struct");
        // }
    }

    // interface用コンストラクタ
    Variable(const std::string &interface_name, bool is_interface_flag)
        : type(TYPE_INTERFACE), is_const(false), is_array(false),
          is_assigned(false), is_multidimensional(false), is_struct(false),
          is_pointer(false), pointer_depth(0), pointer_base_type_name(""),
          pointer_base_type(TYPE_UNKNOWN), is_reference(false),
          is_unsigned(false), struct_type_name(""), is_private_member(false),
          type_name(""), current_type(TYPE_UNKNOWN), value(0), str_value(""),
          float_value(0.0f), double_value(0.0), quad_value(0.0L), big_value(0),
          array_size(0), interface_name(interface_name),
          is_function_pointer(false), function_pointer_name("") {
        // デバッグ用
        extern bool debug_mode;
        if (debug_mode) {
            debug_msg(DebugMsgId::VAR_MANAGER_STRUCT_CREATE,
                      interface_name.c_str(), "interface");
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

// 関数ポインタ情報
struct FunctionPointer {
    const ASTNode *function_node; // 関数定義のASTノード
    std::string function_name;    // 関数名
    TypeInfo return_type;         // 戻り値の型

    FunctionPointer()
        : function_node(nullptr), function_name(""), return_type(TYPE_UNKNOWN) {
    }

    FunctionPointer(const ASTNode *node, const std::string &name,
                    TypeInfo ret_type)
        : function_node(node), function_name(name), return_type(ret_type) {}
};

// スコープ管理
struct Scope {
    std::map<std::string, Variable> variables;
    std::map<std::string, const ASTNode *> functions;
    std::map<std::string, FunctionPointer>
        function_pointers; // 関数ポインタ変数

    void clear() {
        variables.clear();
        functions.clear();
        function_pointers.clear();
    }
};
class ReturnException {
  public:
    int64_t value;
    double double_value;
    long double quad_value;
    std::string str_value;
    TypeInfo type;

    // 配列戻り値サポート
    bool is_array = false;
    std::vector<std::vector<std::vector<int64_t>>> int_array_3d;
    std::vector<std::vector<std::vector<std::string>>> str_array_3d;
    std::vector<std::vector<std::vector<double>>>
        double_array_3d; // float/double配列用
    std::string array_type_name;

    // struct戻り値サポート
    bool is_struct = false;
    Variable struct_value;

    // 構造体配列戻り値サポート
    bool is_struct_array = false;
    std::vector<std::vector<std::vector<Variable>>> struct_array_3d;
    std::string struct_type_name;

    // 参照戻り値サポート
    bool is_reference = false;
    Variable *reference_target = nullptr; // 参照先の変数へのポインタ

    // 関数ポインタ戻り値サポート
    bool is_function_pointer = false;
    std::string function_pointer_name;
    const ASTNode *function_pointer_node = nullptr;

    // 完全初期化コンストラクタ群
    ReturnException(int64_t val, TypeInfo t = TYPE_INT)
        : value(val), double_value(static_cast<double>(val)),
          quad_value(static_cast<long double>(val)), str_value(""), type(t),
          is_array(false), is_struct(false), is_struct_array(false),
          is_reference(false), reference_target(nullptr),
          is_function_pointer(false), function_pointer_name(""),
          function_pointer_node(nullptr) {}
    ReturnException(double val, TypeInfo t = TYPE_DOUBLE)
        : value(static_cast<int64_t>(val)), double_value(val),
          quad_value(static_cast<long double>(val)), str_value(""), type(t),
          is_array(false), is_struct(false), is_struct_array(false),
          is_reference(false), reference_target(nullptr),
          is_function_pointer(false), function_pointer_name(""),
          function_pointer_node(nullptr) {}
    ReturnException(long double val, TypeInfo t = TYPE_QUAD)
        : value(static_cast<int64_t>(val)),
          double_value(static_cast<double>(val)), quad_value(val),
          str_value(""), type(t), is_array(false), is_struct(false),
          is_struct_array(false), is_reference(false),
          reference_target(nullptr), is_function_pointer(false),
          function_pointer_name(""), function_pointer_node(nullptr) {}
    ReturnException(const std::string &str)
        : value(0), double_value(0.0), quad_value(0.0L), str_value(str),
          type(TYPE_STRING), is_array(false), is_struct(false),
          is_struct_array(false), is_reference(false),
          reference_target(nullptr), is_function_pointer(false),
          function_pointer_name(""), function_pointer_node(nullptr) {}

    // 配列戻り値用コンストラクタ
    ReturnException(const std::vector<std::vector<std::vector<int64_t>>> &arr,
                    const std::string &type_name, TypeInfo t)
        : value(0), double_value(0.0), quad_value(0.0L), str_value(""), type(t),
          is_array(true), int_array_3d(arr), array_type_name(type_name),
          is_struct(false), is_struct_array(false), is_reference(false),
          reference_target(nullptr), is_function_pointer(false),
          function_pointer_name(""), function_pointer_node(nullptr) {}

    ReturnException(
        const std::vector<std::vector<std::vector<std::string>>> &arr,
        const std::string &type_name, TypeInfo t)
        : value(0), double_value(0.0), quad_value(0.0L), str_value(""), type(t),
          is_array(true), str_array_3d(arr), array_type_name(type_name),
          is_struct(false), is_struct_array(false), is_reference(false),
          reference_target(nullptr), is_function_pointer(false),
          function_pointer_name(""), function_pointer_node(nullptr) {}

    // float/double配列戻り値用コンストラクタ
    ReturnException(const std::vector<std::vector<std::vector<double>>> &arr,
                    const std::string &type_name, TypeInfo t)
        : value(0), double_value(0.0), quad_value(0.0L), str_value(""), type(t),
          is_array(true), double_array_3d(arr), array_type_name(type_name),
          is_struct(false), is_struct_array(false), is_reference(false),
          reference_target(nullptr), is_function_pointer(false),
          function_pointer_name(""), function_pointer_node(nullptr) {}

    // struct戻り値用コンストラクタ
    ReturnException(const Variable &struct_var)
        : value(0), double_value(0.0), quad_value(0.0L), str_value(""),
          type(struct_var.type), is_array(false), is_struct(true),
          struct_value(struct_var), is_struct_array(false), is_reference(false),
          reference_target(nullptr), is_function_pointer(false),
          function_pointer_name(""), function_pointer_node(nullptr) {}

    // 構造体配列戻り値用コンストラクタ
    ReturnException(
        const std::vector<std::vector<std::vector<Variable>>> &struct_arr,
        const std::string &type_name)
        : value(0), double_value(0.0), quad_value(0.0L), str_value(""),
          type(TYPE_STRUCT), is_array(true), is_struct(true),
          is_struct_array(true), struct_array_3d(struct_arr),
          struct_type_name(type_name), is_reference(false),
          reference_target(nullptr), is_function_pointer(false),
          function_pointer_name(""), function_pointer_node(nullptr) {}

    // 参照戻り値用コンストラクタ
    ReturnException(Variable *ref_target)
        : value(0), double_value(0.0), quad_value(0.0L), str_value(""),
          type(ref_target ? ref_target->type : TYPE_UNKNOWN), is_array(false),
          is_struct(false), is_struct_array(false), is_reference(true),
          reference_target(ref_target), is_function_pointer(false),
          function_pointer_name(""), function_pointer_node(nullptr) {}

    // 関数ポインタ戻り値用コンストラクタ
    ReturnException(int64_t val, const std::string &func_name,
                    const ASTNode *func_node, TypeInfo t)
        : value(val), double_value(static_cast<double>(val)),
          quad_value(static_cast<long double>(val)), str_value(""), type(t),
          is_array(false), is_struct(false), is_struct_array(false),
          is_reference(false), reference_target(nullptr),
          is_function_pointer(true), function_pointer_name(func_name),
          function_pointer_node(func_node) {}
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
    std::map<std::string, StructDefinition>
        struct_definitions_; // struct定義の保存

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
    std::unique_ptr<EnumManager> enum_manager_; // enum管理サービス
    std::unique_ptr<StaticVariableManager>
        static_variable_manager_; // static変数管理
    std::unique_ptr<InterfaceOperations>
        interface_operations_; // interface/impl管理
    std::unique_ptr<StructOperations> struct_operations_; // struct操作管理
    std::unique_ptr<StructVariableManager>
        struct_variable_manager_; // struct変数管理
    std::unique_ptr<ControlFlowExecutor>
        control_flow_executor_; // 制御フロー実行
    std::unique_ptr<StatementListExecutor>
        statement_list_executor_; // 文リスト・複合文実行
    std::unique_ptr<ReturnHandler> return_handler_; // return文処理
    std::unique_ptr<AssertionHandler> assertion_handler_; // アサーション文処理
    std::unique_ptr<BreakContinueHandler>
        break_continue_handler_; // break/continue文処理
    std::unique_ptr<FunctionDeclarationHandler>
        function_declaration_handler_; // 関数宣言処理
    std::unique_ptr<StructDeclarationHandler>
        struct_declaration_handler_; // 構造体宣言処理
    std::unique_ptr<InterfaceDeclarationHandler>
        interface_declaration_handler_; // インターフェース宣言処理
    std::unique_ptr<ImplDeclarationHandler>
        impl_declaration_handler_; // impl宣言処理
    std::unique_ptr<ExpressionStatementHandler>
        expression_statement_handler_; // 式文処理

    // Grant access to managers
    friend class VariableManager;
    friend class ArrayManager;
    friend class TypeManager;
    friend class ArrayProcessingService; // DRY効率化: 配列処理統合サービス
    friend class EnumManager;                // enum管理サービス
    friend class StructOperations;           // struct操作管理
    friend class StructVariableManager;      // struct変数管理
    friend class ControlFlowExecutor;        // 制御フロー実行
    friend class StatementListExecutor;      // 文リスト・複合文実行
    friend class ReturnHandler;              // return文処理
    friend class AssertionHandler;           // アサーション文処理
    friend class BreakContinueHandler;       // break/continue文処理
    friend class FunctionDeclarationHandler; // 関数宣言処理
    friend class StructDeclarationHandler;   // 構造体宣言処理
    friend class InterfaceDeclarationHandler; // インターフェース宣言処理
    friend class ImplDeclarationHandler;      // impl宣言処理
    friend class ExpressionStatementHandler; // 式文処理

  public:
    Interpreter(bool debug = false);
    virtual ~Interpreter();

    // EvaluatorInterface実装
    void process(const ASTNode *ast) override;
    int64_t evaluate(const ASTNode *node) override;
    TypedValue evaluate_typed(const ASTNode *node);

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
    std::string find_variable_name_by_address(const Variable *target_var);

    // 一時変数管理（メソッドチェーン用） (InterfaceOperationsへ委譲)
    void add_temp_variable(const std::string &name, const Variable &var);
    void remove_temp_variable(const std::string &name);
    void clear_temp_variables();
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
    void assign_variable(const std::string &name, const TypedValue &value,
                         TypeInfo type_hint, bool is_const);

    // Union型代入
    void assign_union_variable(const std::string &name,
                               const ASTNode *value_node);

    void assign_function_parameter(const std::string &name, int64_t value,
                                   TypeInfo type, bool is_unsigned);
    void assign_function_parameter(const std::string &name,
                                   const TypedValue &value, TypeInfo type,
                                   bool is_unsigned);
    void assign_array_parameter(const std::string &name,
                                const Variable &source_array, TypeInfo type);
    void assign_interface_view(const std::string &dest_name,
                               Variable interface_var,
                               const Variable &source_var,
                               const std::string &source_var_name);
    void assign_array_element(const std::string &name, int64_t index,
                              int64_t value);
    void assign_array_element_float(const std::string &name, int64_t index,
                                    double value);
    void assign_string_element(const std::string &name, int64_t index,
                               const std::string &value);

    // 配列リテラル割り当て
    void assign_array_literal(const std::string &name,
                              const ASTNode *literal_node);

    // 関数戻り値からの配列割り当て
    void assign_array_from_return(const std::string &name,
                                  const ReturnException &ret);

    // 型解決 (TypeManagerへの薄いラッパー、将来的にはインライン化予定)
    TypeInfo resolve_type_alias(TypeInfo base_type,
                                const std::string &type_name);

    // typedef処理 (TypeManagerへの薄いラッパー、将来的にはインライン化予定)
    std::string resolve_typedef(const std::string &type_name);

    // struct処理
    void register_struct_definition(const std::string &struct_name,
                                    const StructDefinition &definition);
    const StructDefinition *
    find_struct_definition(const std::string &struct_name);

    const ASTNode *find_union_definition(const std::string &union_name);
    const ASTNode *find_typedef_definition(const std::string &typedef_name);
    void create_struct_variable(const std::string &var_name,
                                const std::string &struct_type_name);
    Variable *get_struct_member(const std::string &var_name,
                                const std::string &member_name);
    void assign_struct_literal(const std::string &var_name,
                               const ASTNode *literal_node);
    // 構造体メンバの個別変数を再帰的に作成
    void create_struct_member_variables_recursively(
        const std::string &base_path, const std::string &struct_type_name,
        Variable &parent_var);
    void assign_struct_member(const std::string &var_name,
                              const std::string &member_name, int64_t value);
    void assign_struct_member(const std::string &var_name,
                              const std::string &member_name,
                              const std::string &value);
    void assign_struct_member(const std::string &var_name,
                              const std::string &member_name,
                              const TypedValue &typed_value);
    // 構造体メンバーに構造体を代入
    void assign_struct_member_struct(const std::string &var_name,
                                     const std::string &member_name,
                                     const Variable &struct_value);
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
    int64_t get_struct_member_multidim_array_element(
        const std::string &var_name, const std::string &member_name,
        const std::vector<int64_t> &indices);
    std::string get_struct_member_array_string_element(
        const std::string &var_name, const std::string &member_name, int index);
    void assign_struct_member_array_literal(const std::string &var_name,
                                            const std::string &member_name,
                                            const ASTNode *array_literal);
    // 型名から型情報への変換
    // (TypeManagerへの薄いラッパー、将来的にはインライン化予定)
    TypeInfo string_to_type_info(const std::string &type_str);
    void sync_struct_members_from_direct_access(const std::string &var_name);
    void sync_direct_access_from_struct_value(const std::string &var_name,
                                              const Variable &struct_value);
    void sync_individual_member_from_struct(Variable *struct_var,
                                            const std::string &member_name);
    void ensure_struct_member_access_allowed(const std::string &accessor_name,
                                             const std::string &member_name);
    bool is_current_impl_context_for(const std::string &struct_type_name);

    // static変数処理 (StaticVariableManagerへ委譲)
    Variable *find_static_variable(const std::string &name);
    void create_static_variable(const std::string &name, const ASTNode *node);

    // impl static変数処理 (StaticVariableManagerへ委譲)
    Variable *find_impl_static_variable(const std::string &name);
    void create_impl_static_variable(const std::string &name,
                                     const ASTNode *node);
    void enter_impl_context(const std::string &interface_name,
                            const std::string &struct_type_name);
    void exit_impl_context();
    std::string get_impl_static_namespace() const;

    // interface管理 (InterfaceOperationsへ委譲)
    void register_interface_definition(const std::string &interface_name,
                                       const InterfaceDefinition &definition);
    const InterfaceDefinition *
    find_interface_definition(const std::string &interface_name);

    // impl管理 (InterfaceOperationsへ委譲)
    void register_impl_definition(const ImplDefinition &impl_def);
    const ImplDefinition *
    find_impl_for_struct(const std::string &struct_name,
                         const std::string &interface_name);

    // interface型変数管理 (InterfaceOperationsへ委譲)
    void create_interface_variable(const std::string &var_name,
                                   const std::string &interface_name);
    Variable *get_interface_variable(const std::string &var_name);

    // impl定義へのアクセサ (InterfaceOperationsへ委譲)
    const std::vector<ImplDefinition> &get_impl_definitions() const;

    // 関数コンテキスト
    std::string current_function_name; // 現在実行中の関数名

  private:
    void print_value(const ASTNode *expr);
    void print_formatted(const ASTNode *format_str, const ASTNode *arg_list);
    void validate_struct_recursion_rules();

    // N次元配列リテラル処理の再帰関数
    void process_ndim_array_literal(const ASTNode *literal_node, Variable &var,
                                    TypeInfo elem_type, int &flat_index,
                                    int max_size);

    // impl宣言処理ヘルパー
    void handle_impl_declaration(const ASTNode *node);

  public:
    void check_type_range(TypeInfo type, int64_t value, const std::string &name,
                          bool is_unsigned = false);

    // 関数コンテキストへのアクセス
    const std::string &get_current_function_name() const {
        return current_function_name;
    }

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
    TypedValue evaluate_typed_expression(const ASTNode *node);

    // TypeManagerへのアクセス
    TypeManager *get_type_manager() { return type_manager_.get(); }

    // EnumManagerへのアクセス
    EnumManager *get_enum_manager() { return enum_manager_.get(); }

    // VariableManagerへのアクセス
    VariableManager *get_variable_manager() { return variable_manager_.get(); }

    // スコープへのアクセス（InterfaceOperations用）
    std::vector<Scope> &get_scope_stack() { return scope_stack; }

    // 変数・関数の登録ヘルパー（InterfaceOperations用）
    void add_variable_to_current_scope(const std::string &name,
                                       const Variable &var) {
        current_scope().variables[name] = var;
    }
    void register_function_to_global(const std::string &key,
                                     const ASTNode *func) {
        global_scope.functions[key] = func;
    }

    // 型推論付き三項演算子評価
    TypedValue evaluate_ternary_typed(const ASTNode *node);

    // 関数定義の検索
    const ASTNode *find_function_definition(const std::string &func_name);

    // Parserからのenum定義同期
    void sync_enum_definitions_from_parser(RecursiveParser *parser);

    // Parserからのstruct定義同期
    void sync_struct_definitions_from_parser(RecursiveParser *parser);

    // N次元配列アクセス用のヘルパー関数
    std::string extract_array_name(const ASTNode *node);
    std::vector<int64_t> extract_array_indices(const ASTNode *node);
    std::string extract_array_element_name(const ASTNode *node);

    // Priority 3: 変数ポインターから名前を取得するヘルパー
    std::string find_variable_name(const Variable *var);

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
    void setMultidimensionalArrayElement(Variable &var,
                                         const std::vector<int64_t> &indices,
                                         double value);

    // Priority 3: 統一された文字列配列アクセス (ArrayProcessingService経由)
    std::string
    getMultidimensionalStringArrayElement(Variable &var,
                                          const std::vector<int64_t> &indices);
    void
    setMultidimensionalStringArrayElement(Variable &var,
                                          const std::vector<int64_t> &indices,
                                          const std::string &value);

    // self処理用ヘルパー関数 (InterfaceOperationsへ委譲)
    std::string get_self_receiver_path();
    void sync_self_to_receiver(const std::string &receiver_path);
};
