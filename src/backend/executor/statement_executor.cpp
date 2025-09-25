#include "statement_executor.h"
#include "../interpreter.h"
#include "../error_handler.h"
#include "../array_manager.h"
#include "../../common/debug.h"
#include "../../common/type_alias.h"

StatementExecutor::StatementExecutor(Interpreter& interpreter) : interpreter_(interpreter) {}

void StatementExecutor::execute_statement(const ASTNode *node) {
    execute(node);
}

void StatementExecutor::execute(const ASTNode *node) {
    if (!node) return;

    switch (node->node_type) {
        case ASTNodeType::AST_ASSIGN: {
            execute_assignment(node);
            break;
        }
        case ASTNodeType::AST_VAR_DECL: {
            execute_variable_declaration(node);
            break;
        }
        case ASTNodeType::AST_MULTIPLE_VAR_DECL: {
            execute_multiple_var_decl(node);
            break;
        }
        case ASTNodeType::AST_ARRAY_DECL: {
            execute_array_decl(node);
            break;
        }
        // 他のstatement types（AST_FUNC_DECL, AST_IF_STMT等）は
        // Interpreterクラスで直接処理されるため、ここでは未対応
        default:
            // StatementExecutorが対応していないノード型は
            // Interpreterで処理される想定
            break;
    }
}

void StatementExecutor::execute_assignment(const ASTNode *node) {
    // 右辺が配列リテラルの場合の特別処理
    if (node->right && node->right->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
        if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
            // 変数への配列リテラル代入
            interpreter_.assign_array_literal(node->left->name, node->right.get());
            return;
        } else {
            throw std::runtime_error("Array literal can only be assigned to simple variables");
        }
    }

    if (node->left && node->left->node_type == ASTNodeType::AST_ARRAY_REF) {
        // 配列要素への代入
        int64_t rvalue = interpreter_.evaluate(node->right.get());
        
        // 多次元配列アクセスかチェック
        if (node->left->left && node->left->left->node_type == ASTNodeType::AST_ARRAY_REF) {
            // 多次元配列要素への代入
            std::string var_name = interpreter_.extract_array_name(node->left.get());
            std::vector<int64_t> indices = interpreter_.extract_array_indices(node->left.get());
            
            Variable *var = interpreter_.find_variable(var_name);
            if (!var) {
                throw std::runtime_error("Variable not found: " + var_name);
            }
            
            if (!var->is_multidimensional) {
                throw std::runtime_error("Variable is not a multidimensional array: " + var_name);
            }
            
            interpreter_.setMultidimensionalArrayElement(*var, indices, rvalue);
        } else {
            // 単一次元配列要素への代入
            int64_t index_value = interpreter_.evaluate(node->left->array_index.get());
            int index = static_cast<int>(index_value);
            
            std::string var_name;
            if (node->left->left && node->left->left->node_type == ASTNodeType::AST_VARIABLE) {
                var_name = node->left->left->name;
            } else if (!node->left->name.empty()) {
                var_name = node->left->name;
            } else {
                throw std::runtime_error("Invalid array reference in assignment");
            }
            
            Variable *var = interpreter_.find_variable(var_name);
            if (!var) {
                // 詳細なエラー表示
                print_error_with_ast_location(
                    "Undefined variable '" + var_name + "'", 
                    node);
                    
                throw DetailedErrorException("Undefined variable: " + var_name);
            }
            
            if (var->type == TYPE_STRING) {
                interpreter_.assign_string_element(var_name, index, 
                                                 std::string(1, static_cast<char>(rvalue)));
            } else {
                interpreter_.assign_array_element(var_name, index, rvalue);
            }
        }
    } else {
        // 通常の変数代入
        int64_t value = interpreter_.evaluate(node->right.get());
        if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
            interpreter_.assign_variable(node->name, node->right->str_value);
        } else {
            interpreter_.assign_variable(node->name, value, node->type_info);
        }
    }
}

void StatementExecutor::execute_variable_declaration(const ASTNode *node) {
    Variable var;
    var.type = node->type_info;
    var.is_const = node->is_const;
    var.is_array = false;

    // typedef配列の場合の特別処理
    if (node->array_type_info.base_type != TYPE_UNKNOWN) {
        // ArrayTypeInfoが設定されている場合は配列として処理
        var.is_array = true;
        var.type = node->array_type_info.base_type;
        
        // デバッグ出力
        if (debug_mode) {
            std::cerr << "DEBUG: Setting array for typedef variable " << node->name 
                      << " with base_type=" << var.type << " is_array=" << var.is_array << std::endl;
        }
        
        // 配列サイズ情報をコピー
        for (const auto& dim : node->array_type_info.dimensions) {
            var.array_dimensions.push_back(dim.size);
            if (debug_mode) {
                std::cerr << "DEBUG: Adding dimension size=" << dim.size << std::endl;
            }
        }
        
        // 配列初期化
        if (!var.array_dimensions.empty()) {
            int total_size = 1;
            for (int dim : var.array_dimensions) {
                total_size *= dim;
            }
            var.array_values.resize(total_size, 0);
            if (debug_mode) {
                std::cerr << "DEBUG: Initialized array with total_size=" << total_size << std::endl;
            }
        }
    }

    // 型を確定する
    if (node->type_info == TYPE_UNKNOWN && !node->str_value.empty()) {
        // 単純な型エイリアス解決
        var.type = TYPE_INT; // デフォルト
    } else if (!var.is_array) {  // 配列でない場合のみ設定
        var.type = node->type_info;
    }

    // 初期化（init_exprまたはrightを使用）
    ASTNode* init_node = node->init_expr ? node->init_expr.get() : node->right.get();
    
    // 変数を現在のスコープに登録（配列リテラル代入前に必要）
    interpreter_.current_scope().variables[node->name] = var;
    
    if (init_node) {
        if (var.is_array && init_node->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
            // 配列リテラル初期化
            interpreter_.assign_array_literal(node->name, init_node);
            // 代入後に変数を再取得して更新
            interpreter_.current_scope().variables[node->name].is_assigned = true;
        } else if (var.is_array && init_node->node_type == ASTNodeType::AST_FUNC_CALL) {
            // 配列を返す関数呼び出し
            try {
                int64_t value = interpreter_.evaluate(init_node);
                // void関数の場合
                interpreter_.current_scope().variables[node->name].value = value;
                interpreter_.current_scope().variables[node->name].is_assigned = true;
            } catch (const ReturnException& ret) {
                if (ret.is_array) {
                    // 配列戻り値の場合
                    Variable& target_var = interpreter_.current_scope().variables[node->name];
                    
                    if (ret.type == TYPE_STRING) {
                        // 文字列配列
                        if (!ret.str_array_3d.empty() && 
                            !ret.str_array_3d[0].empty() && 
                            !ret.str_array_3d[0][0].empty()) {
                            target_var.array_strings = ret.str_array_3d[0][0];
                            target_var.array_size = target_var.array_strings.size();
                            target_var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING);
                        }
                    } else {
                        // 数値配列
                        if (!ret.int_array_3d.empty() && 
                            !ret.int_array_3d[0].empty() && 
                            !ret.int_array_3d[0][0].empty()) {
                            target_var.array_values = ret.int_array_3d[0][0];
                            target_var.array_size = target_var.array_values.size();
                            target_var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + ret.type);
                        }
                    }
                    target_var.is_assigned = true;
                } else {
                    // 非配列戻り値の場合
                    if (ret.type == TYPE_STRING) {
                        interpreter_.current_scope().variables[node->name].str_value = ret.str_value;
                    } else {
                        interpreter_.current_scope().variables[node->name].value = ret.value;
                    }
                    interpreter_.current_scope().variables[node->name].is_assigned = true;
                }
            }
        } else {
            // 通常の初期化
            if (init_node->node_type == ASTNodeType::AST_FUNC_CALL) {
                try {
                    int64_t value = interpreter_.evaluate(init_node);
                    if (var.type == TYPE_STRING) {
                        // 文字列型なのに数値が返された場合
                        throw std::runtime_error("Type mismatch: expected string but got numeric value");
                    } else {
                        interpreter_.current_scope().variables[node->name].value = value;
                    }
                    interpreter_.current_scope().variables[node->name].is_assigned = true;
                } catch (const ReturnException& ret) {
                    if (ret.type == TYPE_STRING) {
                        interpreter_.current_scope().variables[node->name].str_value = ret.str_value;
                        interpreter_.current_scope().variables[node->name].type = TYPE_STRING;
                    } else {
                        interpreter_.current_scope().variables[node->name].value = ret.value;
                    }
                    interpreter_.current_scope().variables[node->name].is_assigned = true;
                }
            } else {
                int64_t value = interpreter_.evaluate(init_node);
                if (var.type == TYPE_STRING) {
                    interpreter_.current_scope().variables[node->name].str_value = init_node->str_value;
                } else {
                    interpreter_.current_scope().variables[node->name].value = value;
                    // interpreter_.check_type_range(var.type, value, node->name);
                }
                interpreter_.current_scope().variables[node->name].is_assigned = true;
            }
        }
    }
}

void StatementExecutor::execute_multiple_var_decl(const ASTNode *node) {
    // 複数変数宣言の処理
    for (const auto &child : node->children) {
        if (child->node_type == ASTNodeType::AST_VAR_DECL) {
            execute_variable_declaration(child.get());
        }
    }
}

void StatementExecutor::execute_array_decl(const ASTNode *node) {
    // 配列宣言をArrayManagerに委譲
    Variable var;
    interpreter_.get_array_manager()->processArrayDeclaration(var, node);
    
    // 変数を現在のスコープに登録
    interpreter_.current_scope().variables[node->name] = var;
}
