#pragma once
#include "../common/ast.h"
#include "evaluator/expression_evaluator.h"
#include "executor/statement_executor.h"
#include "modules/module_resolver.h"
#include "output/output_manager.h"
#include "variables/variable_manager.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

// 変数・関数の格納構造
struct Variable {
    TypeInfo type;
    bool is_const;
    bool is_array;
    bool is_assigned;

    // 値
    int64_t value;
    std::string str_value;

    // 配列用
    int array_size;
    std::vector<int64_t> array_values;
    std::vector<std::string> array_strings;

    Variable()
        : type(TYPE_INT), is_const(false), is_array(false), is_assigned(false),
          value(0), array_size(0) {}
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

    // ヘルパーメソッド
    void register_global_declarations(const ASTNode *node);

  public:
    Interpreter(bool debug = false);
    virtual ~Interpreter() = default;

    // EvaluatorInterface実装
    void process(const ASTNode *ast) override;
    int64_t evaluate(const ASTNode *node) override;

    // デバッグ機能
    void set_debug_mode(bool debug) { debug_mode = debug; }

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
