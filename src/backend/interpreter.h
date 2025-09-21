#pragma once
#include "../common/ast.h"
#include "../common/debug_messages.h"
#include "../common/type_alias.h"
#include "evaluator/expression_evaluator.h"
#include "executor/statement_executor.h"
#include "memory/array_memory_manager.h"
#include "modules/module_resolver.h"
#include "output/output_manager.h"
#include "variables/semantic_analyzer.h"
#include "variables/variable_manager.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

// 前方宣言
class ArrayMemoryBlock;

struct Variable {
    TypeInfo type;
    std::string name;
    std::string string_value;
    int64_t int_value;

    // 配列データ：2つのシステムをサポート
    // Array management members
    ArrayMemoryBlock *array_block;
    std::string array_name;
    std::unique_ptr<std::vector<int64_t>> array_values_ptr;
    std::unique_ptr<std::vector<std::string>> array_strings_ptr;

    int array_size;
    bool is_array;
    bool use_new_array_system; // どちらのシステムを使うか

    // Array typedef サポート
    ArrayTypeInfo array_type_info;

    bool is_const;
    bool is_assigned;

    Variable()
        : type(TYPE_INT), int_value(0), array_block(nullptr), array_name(""),
          array_values_ptr(nullptr), array_strings_ptr(nullptr), array_size(0),
          is_array(false), use_new_array_system(true), is_const(false),
          is_assigned(false) {}

    // ムーブコンストラクタ
    Variable(Variable &&other) noexcept
        : type(other.type), name(std::move(other.name)),
          string_value(std::move(other.string_value)),
          int_value(other.int_value), array_block(other.array_block),
          array_name(std::move(other.array_name)),
          array_values_ptr(std::move(other.array_values_ptr)),
          array_strings_ptr(std::move(other.array_strings_ptr)),
          array_size(other.array_size), is_array(other.is_array),
          use_new_array_system(other.use_new_array_system),
          array_type_info(other.array_type_info), is_const(other.is_const),
          is_assigned(other.is_assigned) {
        other.array_block = nullptr;
    }

    // ムーブ代入演算子
    Variable &operator=(Variable &&other) noexcept {
        if (this != &other) {
            type = other.type;
            name = std::move(other.name);
            string_value = std::move(other.string_value);
            int_value = other.int_value;
            array_block = other.array_block;
            array_name = std::move(other.array_name);
            array_values_ptr = std::move(other.array_values_ptr);
            array_strings_ptr = std::move(other.array_strings_ptr);
            array_size = other.array_size;
            is_array = other.is_array;
            use_new_array_system = other.use_new_array_system;
            array_type_info = other.array_type_info;
            is_const = other.is_const;
            is_assigned = other.is_assigned;
            other.array_block = nullptr;
        }
        return *this;
    }

    // コピーコンストラクタとコピー代入演算子を削除
    Variable(const Variable &) = delete;
    Variable &operator=(const Variable &) = delete;

    // 旧システム用の後方互換アクセサ
    std::vector<int64_t> &array_values() {
        if (!array_values_ptr) {
            array_values_ptr = std::make_unique<std::vector<int64_t>>();
        }
        return *array_values_ptr;
    }

    std::vector<std::string> &array_strings() {
        if (!array_strings_ptr) {
            array_strings_ptr = std::make_unique<std::vector<std::string>>();
        }
        return *array_strings_ptr;
    }
};

// スコープ管理
struct Scope {
    std::map<std::string, Variable> variables;
    std::map<std::string, const ASTNode *> functions;

    // デフォルトコンストラクタ
    Scope() = default;

    // ムーブコンストラクタ
    Scope(Scope &&other) noexcept
        : variables(std::move(other.variables)),
          functions(std::move(other.functions)) {}

    // ムーブ代入演算子
    Scope &operator=(Scope &&other) noexcept {
        if (this != &other) {
            variables = std::move(other.variables);
            functions = std::move(other.functions);
        }
        return *this;
    }

    // コピーコンストラクタとコピー代入演算子を削除
    Scope(const Scope &) = delete;
    Scope &operator=(const Scope &) = delete;

    void clear() {
        variables.clear();
        functions.clear();
    }
};

// 例外クラス
class ReturnException {
  public:
    int64_t value;
    std::string str_value;
    TypeInfo type;

    ReturnException(int64_t val, TypeInfo t = TYPE_INT) : value(val), type(t) {}
    ReturnException(const std::string &str)
        : value(0), str_value(str), type(TYPE_STRING) {}
};

class BreakException {
  public:
    int64_t condition;
    BreakException(int64_t cond = 1) : condition(cond) {}
};

// インタープリター実装
class Interpreter : public EvaluatorInterface {
  private:
    std::vector<Scope> scope_stack;
    Scope global_scope;
    bool debug_mode;
    std::unique_ptr<OutputManager> output_manager_;
    std::unique_ptr<ExpressionEvaluator> expression_evaluator_;
    std::unique_ptr<StatementExecutor> statement_executor_;
    std::unique_ptr<VariableManager> variable_manager_;
    std::unique_ptr<ModuleResolver> module_resolver_;
    std::unique_ptr<SemanticAnalyzer> semantic_analyzer_;

    // ヘルパーメソッド
    void register_global_declarations(const ASTNode *node);

    // 新しい意味解析フェーズ
    bool perform_semantic_analysis(const ASTNode *ast);

  public:
    Interpreter(bool debug = false);
    virtual ~Interpreter() = default;

    // EvaluatorInterface実装
    void process(const ASTNode *ast) override;
    int64_t evaluate(const ASTNode *node) override;

    // デバッグ機能
    void set_debug_mode(bool debug) { debug_mode = debug; }
    bool get_debug_mode() const { return debug_mode; }

    // ExpressionEvaluator用のpublicアクセサ
    Variable *find_variable(const std::string &name);
    const ASTNode *find_function(const std::string &name);
    void check_type_range(TypeInfo type, int64_t value,
                          const std::string &name);
    void push_scope();
    void pop_scope();
    Scope &current_scope();
    void execute_statement(const ASTNode *node);
    int64_t evaluate_expression(const ASTNode *node);

    // StatementExecutor用のpublicアクセサ
    Scope &get_global_scope() { return global_scope; }
    OutputManager &get_output_manager() { return *output_manager_; }
    void assign_variable(const std::string &name, int64_t value,
                         TypeInfo type = TYPE_INT);
    void assign_variable(const std::string &name, const std::string &value,
                         bool is_const);
    void assign_variable(const std::string &name, int64_t value, TypeInfo type,
                         bool is_const);
    void assign_variable(const std::string &name, const std::string &value);
    void assign_array_element(const std::string &name, int64_t index,
                              int64_t value);
    void assign_string_element(const std::string &name, int64_t index,
                               const std::string &value);
    void assign_array_literal(const std::string &name, ASTNode *array_literal);

    // 型エイリアス解決
    TypeInfo resolve_type_alias(TypeInfo type_info,
                                const std::string &type_name = "");

    // OutputManager用のpublicアクセサ
    Variable *get_variable(const std::string &name) {
        return find_variable(name);
    }
    const ASTNode *get_function(const std::string &name) {
        return find_function(name);
    }
    int64_t eval_expression(const ASTNode *node) {
        return evaluate_expression(node);
    }
    void push_interpreter_scope() { push_scope(); }
    void pop_interpreter_scope() { pop_scope(); }
    void exec_statement(const ASTNode *node) { execute_statement(node); }
    Scope &get_current_scope() { return current_scope(); }

    // モジュールシステムサポート
    bool process_import(const ASTNode *import_node);
    ModuleResolver *get_module_resolver() const {
        return module_resolver_.get();
    }

    // モジュール関数サポート
    bool is_module_loaded(const std::string &module_name);
    const ASTNode *find_module_function(const std::string &module_name,
                                        const std::string &function_name);
    int64_t find_module_variable(const std::string &module_name,
                                 const std::string &variable_name);
};
