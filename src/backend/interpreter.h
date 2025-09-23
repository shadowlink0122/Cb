#pragma once
#include "../common/ast.h"
#include "output/output_manager.h"
#include <iostream>
#include <map>
#include <string>
#include <vector>

// 前方宣言
class OutputManager;

// 変数・関数の格納構造
struct Variable {
    TypeInfo type;
    bool is_const;
    bool is_array;
    bool is_assigned;
    bool is_multidimensional; // 多次元配列フラグ

    // 値
    int64_t value;
    std::string str_value;

    // 配列用
    int array_size;
    std::vector<int64_t> array_values;
    std::vector<std::string> array_strings;

    // 多次元配列用
    ArrayTypeInfo array_type_info;     // 多次元配列の型情報
    std::vector<int> array_dimensions; // 各次元のサイズ
    std::vector<int64_t>
        multidim_array_values; // 多次元配列データ（フラット化）
    std::vector<std::string> multidim_array_strings; // 多次元文字列配列データ

    Variable()
        : type(TYPE_INT), is_const(false), is_array(false), is_assigned(false),
          is_multidimensional(false), value(0), array_size(0) {}

    // 多次元配列用コンストラクタ
    Variable(const ArrayTypeInfo &array_info)
        : type(TYPE_INT), is_const(false), is_array(true), is_assigned(false),
          is_multidimensional(true), value(0), array_size(0),
          array_type_info(array_info) {}

    // 多次元配列のフラットインデックス計算
    int calculate_flat_index(const std::vector<int> &indices) const {
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
    OutputManager output_manager_;
    std::map<std::string, std::string>
        typedef_map; // typedef alias -> base type mapping

  public:
    Interpreter(bool debug = false);
    virtual ~Interpreter() = default;

    // EvaluatorInterface実装
    void process(const ASTNode *ast) override;
    int64_t evaluate(const ASTNode *node) override;

    // 出力管理
    OutputManager &get_output_manager() { return output_manager_; }

    // スコープ管理
    void push_scope();
    void pop_scope();
    void push_interpreter_scope() { push_scope(); }
    void pop_interpreter_scope() { pop_scope(); }
    Scope &current_scope();
    Scope &get_current_scope() { return current_scope(); }

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
    void execute_statement(const ASTNode *node);
    void exec_statement(const ASTNode *node) { execute_statement(node); }
    int64_t evaluate_expression(const ASTNode *node);
    int64_t eval_expression(const ASTNode *node) {
        return evaluate_expression(node);
    }

    // 変数操作 (publicに移動)
    void assign_variable(const std::string &name, int64_t value,
                         TypeInfo type = TYPE_INT);
    void assign_variable(const std::string &name, const std::string &value);
    void assign_variable(const std::string &name, int64_t value, TypeInfo type,
                         bool is_const);
    void assign_variable(const std::string &name, const std::string &value,
                         bool is_const);
    void assign_array_element(const std::string &name, int64_t index,
                              int64_t value);
    void assign_string_element(const std::string &name, int64_t index,
                               const std::string &value);

    // 配列リテラル割り当て（プレースホルダー）
    void assign_array_literal(const std::string &name,
                              const ASTNode *literal_node) {
        // TODO: 実装が必要
    }

    // 型解決
    TypeInfo resolve_type_alias(TypeInfo base_type,
                                const std::string &type_name) {
        // TODO: 実装が必要
        return base_type;
    }

    // typedef処理
    std::string resolve_typedef(const std::string &type_name);
    TypeInfo string_to_type_info(const std::string &type_str);

  private:
    void print_value(const ASTNode *expr);
    void print_formatted(const ASTNode *format_str, const ASTNode *arg_list);

  public:
    void check_type_range(TypeInfo type, int64_t value,
                          const std::string &name);

    // デバッグ機能
    void set_debug_mode(bool debug) { debug_mode = debug; }
};
