#include "executor/statement_executor.h"
#include "services/array_processing_service.h"
#include "core/interpreter.h"
#include "core/type_inference.h"
#include "core/error_handler.h"
#include "core/pointer_metadata.h"
#include "evaluator/expression_evaluator.h"
#include "managers/variable_manager.h"
#include "managers/array_manager.h"
#include "managers/type_manager.h"
#include "../../../common/debug.h"
#include "../../../common/type_alias.h"

StatementExecutor::StatementExecutor(Interpreter& interpreter) : interpreter_(interpreter) {}

void StatementExecutor::execute_statement(const ASTNode *node) {
    execute(node);
}

void StatementExecutor::execute(const ASTNode *node) {
    if (!node) return;
    
    // ASTNodeTypeが異常な値でないことを確認
    int node_type_int = static_cast<int>(node->node_type);
    if (node_type_int < 0 || node_type_int > 100) {
        debug_msg(DebugMsgId::INTERPRETER_EXEC_STMT, 
                  "Abnormal node_type detected: %d, skipping execution", node_type_int);
        if (debug_mode) {
            std::cerr << "[CRITICAL] Abnormal node_type detected: " << node_type_int 
                      << " (ptr: " << static_cast<const void*>(node) << ")" << std::endl;
            std::cerr << "[CRITICAL] Node name: '" << node->name << "'" << std::endl;
        }
        return;
    }
    
    if (debug_mode) {
        std::cerr << "[DEBUG_EXECUTE] Executing node type: " << node_type_int << std::endl;
    }

    switch (node->node_type) {
        case ASTNodeType::AST_ASSIGN: {
            execute_assignment(node);
            break;
        }
        case ASTNodeType::AST_VAR_DECL: {
            // Debug output removed - use --debug option if needed
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
        case ASTNodeType::AST_PRE_INCDEC:
        case ASTNodeType::AST_POST_INCDEC: {
            // インクリメント/デクリメントをステートメントとして実行
            // expression_evaluatorで評価するだけで副作用（変数の変更）が発生する
            interpreter_.evaluate(node);
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
    // 間接参照への代入 (*ptr = value)
    if (node->left && node->left->node_type == ASTNodeType::AST_UNARY_OP && 
        node->left->op == "DEREFERENCE") {
        // ポインタを評価
        int64_t ptr_value = interpreter_.evaluate(node->left->left.get());
        if (ptr_value == 0) {
            throw std::runtime_error("Null pointer dereference in assignment");
        }
        
        // 右辺を評価
        int64_t value = interpreter_.evaluate(node->right.get());
        
        // ポインタがメタデータを持つかチェック
        if (ptr_value & (1LL << 63)) {
            // メタデータポインタの場合
            int64_t clean_ptr = ptr_value & ~(1LL << 63);
            using namespace PointerSystem;
            PointerMetadata* meta = reinterpret_cast<PointerMetadata*>(clean_ptr);
            
            if (!meta) {
                throw std::runtime_error("Invalid pointer metadata in assignment");
            }
            
            if (debug_mode) {
                std::cerr << "[POINTER_METADATA] Assignment through pointer: " 
                          << meta->to_string() << " = " << value << std::endl;
            }
            
            // メタデータを通じて値を書き込み
            meta->write_int_value(value);
        } else {
            // 従来の方式（変数ポインタ）
            Variable *var = reinterpret_cast<Variable*>(ptr_value);
            var->value = value;
            var->is_assigned = true;
        }
        return;
    }
    
    // 右辺が三項演算子の場合の特別処理
    if (node->right && node->right->node_type == ASTNodeType::AST_TERNARY_OP) {
        execute_ternary_assignment(node);
        return;
    }

    // 右辺が配列リテラルの場合の特別処理
    if (node->right && node->right->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
        if (node->left && node->left->node_type == ASTNodeType::AST_VARIABLE) {
            // 通常の変数への配列リテラル代入
            std::string var_name = node->left->name;
            interpreter_.assign_array_literal(var_name, node->right.get());
            return;
        } else if (node->left && node->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
            // 構造体メンバーへの配列リテラル代入 (obj.member = [1, 2, 3])
            execute_member_array_literal_assignment(node);
            return;
        } else if (!node->name.empty()) {
            // 名前による直接代入
            interpreter_.assign_array_literal(node->name, node->right.get());
            return;
        } else {
            throw std::runtime_error("Array literal can only be assigned to variables or struct members");
        }
    }

    // 右辺が構造体リテラルの場合の特別処理
    if (node->right && node->right->node_type == ASTNodeType::AST_STRUCT_LITERAL) {
        if (!node->left) {
            // leftがnullの場合、nameに変数名が入っている可能性がある
            if (!node->name.empty()) {
                interpreter_.assign_struct_literal(node->name, node->right.get());
                return;
            }
            throw std::runtime_error("Assignment left side is null and name is empty");
        }
        
        if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
            // 変数への構造体リテラル代入
            Variable* var = interpreter_.get_variable(node->left->name);
            
            if (var && var->is_array) {
                throw std::runtime_error("Array assignment must use [] syntax, not {}");
            }
            
            interpreter_.assign_struct_literal(node->left->name, node->right.get());
            return;
        } else if (node->left->node_type == ASTNodeType::AST_ARRAY_REF) {
            // 配列要素への構造体リテラル代入 (team[0] = {})
            if (debug_mode) {
                std::cerr << "DEBUG: Struct literal assignment to array element" << std::endl;
            }
            std::string element_name = interpreter_.extract_array_element_name(node->left.get());
            if (debug_mode) {
                std::cerr << "DEBUG: Array element name: " << element_name << std::endl;
            }
            interpreter_.assign_struct_literal(element_name, node->right.get());
            return;
        } else {
            throw std::runtime_error("Struct literal can only be assigned to variables or array elements");
        }
    }

    if (node->left && node->left->node_type == ASTNodeType::AST_ARRAY_REF) {
        // 配列要素への代入
        
        // 右辺が構造体戻り値関数の場合の特別処理
        if (node->right && node->right->node_type == ASTNodeType::AST_FUNC_CALL) {
            try {
                // 関数呼び出し結果を評価（戻り値は現在未使用だが、副作用のため実行）
                interpreter_.evaluate(node->right.get());
                // 通常の数値戻り値の場合は通常処理を継続
            } catch (const ReturnException& ret) {
                if (ret.is_struct) {
                    // 構造体戻り値を配列要素に代入
                    std::string element_name = interpreter_.extract_array_element_name(node->left.get());
                    debug_msg(DebugMsgId::INTERPRETER_STRUCT_REGISTERED, 
                              "Assigning struct return value to array element: %s", element_name.c_str());
                    
                    debug_print("ReturnException struct_value: struct_members.size() = %zu\n", 
                               ret.struct_value.struct_members.size());
                    
                    // 構造体変数を作成・代入
                    interpreter_.current_scope().variables[element_name] = ret.struct_value;
                    
                    Variable& assigned_var = interpreter_.current_scope().variables[element_name];
                    debug_print("Assigned variable: struct_members.size() = %zu\n", 
                               assigned_var.struct_members.size());
                    
                    // 個別メンバー変数も更新する必要がある
                    for (const auto& member : assigned_var.struct_members) {
                        std::string member_path = element_name + "." + member.first;
                        Variable* member_var = interpreter_.find_variable(member_path);
                        if (member_var) {
                            member_var->value = member.second.value;
                            member_var->str_value = member.second.str_value;
                            member_var->is_assigned = true;
                            debug_print("Updated member variable: %s = %lld\n", 
                                       member_path.c_str(), member.second.value);
                        }
                    }
                    
                    return;
                } else {
                    // その他の戻り値は再投げ
                    throw;
                }
            }
        }
        
        // 右辺の評価（構造体変数やその他の式）
        // TypedValueで評価して、float/doubleにも対応
        int64_t rvalue = 0;
        bool is_floating = false;
        double float_rvalue = 0.0;
        
        try {
            TypedValue typed_rvalue = interpreter_.evaluate_typed_expression(node->right.get());
            if (typed_rvalue.is_floating()) {
                is_floating = true;
                float_rvalue = typed_rvalue.as_double();
                rvalue = static_cast<int64_t>(float_rvalue);
            } else {
                rvalue = typed_rvalue.as_numeric();
            }
        } catch (const ReturnException& ret) {
            if (ret.is_struct) {
                // 構造体変数または構造体戻り値を配列要素に代入
                std::string element_name = interpreter_.extract_array_element_name(node->left.get());
                debug_msg(DebugMsgId::INTERPRETER_STRUCT_REGISTERED, 
                          "Assigning struct variable/return value to array element: %s", element_name.c_str());
                
                std::cerr << "DEBUG: Struct assignment to array element: " << element_name << std::endl;
                std::cerr << "DEBUG: struct_type_name=" << ret.struct_value.struct_type_name << std::endl;
                std::cerr << "DEBUG: struct_members.size()=" << ret.struct_value.struct_members.size() << std::endl;
                
                // 構造体データをデバッグ
                for (const auto& member : ret.struct_value.struct_members) {
                    std::cerr << "DEBUG: member[" << member.first << "] = " << member.second.value 
                             << " (assigned=" << member.second.is_assigned << ")" << std::endl;
                }
                
                // 構造体変数を作成・代入
                interpreter_.current_scope().variables[element_name] = ret.struct_value;
                
                Variable& assigned_var = interpreter_.current_scope().variables[element_name];
                
                // 個別メンバー変数も更新する必要がある
                for (const auto& member : assigned_var.struct_members) {
                    std::string member_path = element_name + "." + member.first;
                    Variable* member_var = interpreter_.find_variable(member_path);
                    if (member_var) {
                        member_var->value = member.second.value;
                        member_var->str_value = member.second.str_value;
                        member_var->is_assigned = member.second.is_assigned;
                        std::cerr << "DEBUG: Updated member variable: " << member_path 
                                 << " = " << member.second.value 
                                 << " (assigned=" << member.second.is_assigned << ")" << std::endl;
                    }
                }
                
                return;
            } else {
                // その他の戻り値は再投げ
                throw;
            }
        }
        
        // 構造体メンバーの1次元配列アクセスかチェック: obj.member[i] = value
        if (node->left->left && node->left->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
            // obj.member[index] = value のケース
            debug_print("DEBUG: Detected struct member 1D array assignment\n");
            std::string obj_name;
            if (node->left->left->left && 
                (node->left->left->left->node_type == ASTNodeType::AST_VARIABLE ||
                 node->left->left->left->node_type == ASTNodeType::AST_IDENTIFIER)) {
                obj_name = node->left->left->left->name;
                debug_print("DEBUG: obj_name = %s\n", obj_name.c_str());
            } else {
                if (node->left->left->left) {
                    debug_print("ERROR: Invalid node type for object: %d\n", 
                               static_cast<int>(node->left->left->left->node_type));
                } else {
                    debug_print("ERROR: node->left->left->left is null\n");
                }
                throw std::runtime_error("Invalid object reference in member array access");
            }
            std::string member_name = node->left->left->name;
            int64_t index = interpreter_.evaluate(node->left->array_index.get());
            
            // 右辺を評価
            if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
                interpreter_.assign_struct_member_array_element(obj_name, member_name, index, node->right->str_value);
            } else {
                TypedValue typed_value = interpreter_.evaluate_typed(node->right.get());
                if (typed_value.is_floating()) {
                    interpreter_.assign_struct_member_array_element(obj_name, member_name, index, typed_value.as_double());
                } else {
                    interpreter_.assign_struct_member_array_element(obj_name, member_name, index, typed_value.as_numeric());
                }
            }
            return;
        }
        
        // 多次元配列アクセスかチェック
        if (node->left->left && node->left->left->node_type == ASTNodeType::AST_ARRAY_REF) {
            // 構造体メンバーの多次元配列かチェック
            ASTNode* deepest_left = node->left.get();
            while (deepest_left->left && deepest_left->left->node_type == ASTNodeType::AST_ARRAY_REF) {
                deepest_left = deepest_left->left.get();
            }
            
            if (deepest_left->left && deepest_left->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
                // 構造体メンバーの多次元配列への代入: obj.member[i][j] = value
                debug_msg(DebugMsgId::MULTIDIM_ARRAY_ASSIGNMENT_DETECTED);
                std::string obj_name = deepest_left->left->left->name;
                std::string member_name = deepest_left->left->name;
                debug_msg(DebugMsgId::VAR_MANAGER_STRUCT_CREATE, obj_name.c_str(), member_name.c_str());
                
                // インデックスを収集（ネストしたAST_ARRAY_REFから）
                std::vector<int64_t> indices;
                ASTNode* current_ref = node->left.get();
                
                // 逆順でインデックスを収集（最外層から最内層へ）
                std::vector<const ASTNode*> refs;
                while (current_ref && current_ref->node_type == ASTNodeType::AST_ARRAY_REF) {
                    refs.push_back(current_ref);
                    current_ref = current_ref->left.get();
                }
                
                // 順序を逆にして、インデックスを正しい順序で収集
                for (auto it = refs.rbegin(); it != refs.rend(); ++it) {
                    if (!(*it)->array_index) {
                        std::cerr << "[ERROR] Null array_index in nested AST_ARRAY_REF" << std::endl;
                        std::cerr << "[ERROR] Node type: " << (int)(*it)->node_type << std::endl;
                        std::cerr << "[ERROR] This suggests parser failed to properly construct AST for multidimensional access" << std::endl;
                        throw std::runtime_error("Null array_index in multidimensional access");
                    }
                    debug_msg(DebugMsgId::ARRAY_ELEMENT_EVAL_START);
                    int64_t index = interpreter_.evaluate((*it)->array_index.get());
                    std::string index_str = std::to_string(index);
                    debug_msg(DebugMsgId::ARRAY_ELEMENT_EVAL_VALUE, index_str.c_str());
                    indices.push_back(index);
                }
                
                // 構造体メンバー変数を取得
                Variable *member_var = interpreter_.get_struct_member(obj_name, member_name);
                if (!member_var) {
                    throw std::runtime_error("Struct member not found: " + member_name);
                }
                
                // 多次元配列の要素に代入
                if (member_var->is_multidimensional && indices.size() > 1) {
                    // float/doubleの場合は対応するオーバーロードを使用
                    TypeInfo base_type = member_var->array_type_info.base_type;
                    if (is_floating && (base_type == TYPE_FLOAT || base_type == TYPE_DOUBLE || base_type == TYPE_QUAD)) {
                        interpreter_.setMultidimensionalArrayElement(*member_var, indices, float_rvalue);
                    } else {
                        interpreter_.setMultidimensionalArrayElement(*member_var, indices, rvalue);
                    }
                } else {
                    throw std::runtime_error("Invalid multidimensional member array access");
                }
            } else {
                // 通常の多次元配列要素への代入
                std::string var_name = interpreter_.extract_array_name(node->left.get());
                std::vector<int64_t> indices = interpreter_.extract_array_indices(node->left.get());
                
                Variable *var = interpreter_.find_variable(var_name);
                if (!var) {
                    throw std::runtime_error("Variable not found: " + var_name);
                }
                
                if (!var->is_multidimensional) {
                    throw std::runtime_error("Variable is not a multidimensional array: " + var_name);
                }
                
                // float/doubleの場合は対応するオーバーロードを使用
                TypeInfo base_type = var->array_type_info.base_type;
                if (is_floating && (base_type == TYPE_FLOAT || base_type == TYPE_DOUBLE || base_type == TYPE_QUAD)) {
                    interpreter_.setMultidimensionalArrayElement(*var, indices, float_rvalue);
                } else {
                    interpreter_.setMultidimensionalArrayElement(*var, indices, rvalue);
                }
            }
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
                // float/double/quad配列の場合はfloat値を使用
                TypeInfo base_type = (var->type >= TYPE_ARRAY_BASE) 
                                    ? static_cast<TypeInfo>(var->type - TYPE_ARRAY_BASE) 
                                    : var->type;
                if (is_floating && (base_type == TYPE_FLOAT || base_type == TYPE_DOUBLE || base_type == TYPE_QUAD)) {
                    interpreter_.assign_array_element_float(var_name, index, float_rvalue);
                } else {
                    interpreter_.assign_array_element(var_name, index, rvalue);
                }
            }
        }
    } else if (node->left && node->left->node_type == ASTNodeType::AST_MEMBER_ARRAY_ACCESS) {
        // メンバの配列アクセスへの代入 (obj.member[index] = value)
        debug_print("DEBUG: Detected AST_MEMBER_ARRAY_ACCESS assignment\n");
        execute_member_array_assignment(node);
    } else if (node->left && node->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
        // メンバアクセスへの代入 (obj.member = value)
        execute_member_assignment(node);
    } else if (node->left && node->left->node_type == ASTNodeType::AST_ARROW_ACCESS) {
        // アロー演算子アクセスへの代入 (ptr->member = value)
        execute_arrow_assignment(node);
    } else {
        // 通常の変数代入
        // Union型変数への代入の特別処理
        if (node->left && node->left->node_type == ASTNodeType::AST_VARIABLE) {
            Variable* var = interpreter_.find_variable(node->left->name);
            if (var && var->type == TYPE_UNION) {
                interpreter_.assign_union_variable(node->left->name, node->right.get());
                return;
            }
        }

        // node->nameベースの代入でもUnion型チェック
        if (!node->name.empty()) {
            Variable* var = interpreter_.find_variable(node->name);
            if (var && var->type == TYPE_UNION) {
                interpreter_.assign_union_variable(node->name, node->right.get());
                return;
            }
        }

        std::string target_name = !node->name.empty()
                                       ? node->name
                                       : (node->left &&
                                                  node->left->node_type ==
                                                      ASTNodeType::AST_VARIABLE
                                              ? node->left->name
                                              : "");

        if (target_name.empty()) {
            throw std::runtime_error("Invalid assignment target");
        }

        Variable *target_var = interpreter_.find_variable(target_name);

        // Interface型変数（ポインタを除く）への代入処理
        if (target_var &&
            (target_var->type == TYPE_INTERFACE ||
             !target_var->interface_name.empty()) &&
            target_var->type != TYPE_POINTER) {
            auto assign_from_source = [&](const Variable &source,
                                          const std::string &source_name) {
                interpreter_.get_variable_manager()->assign_interface_view(
                    target_name, *target_var, source, source_name);
            };

            auto create_temp_from_typed = [&](const TypedValue &typed,
                                              TypeInfo type_hint) -> Variable {
                Variable temp;
                temp.is_assigned = true;
                temp.struct_type_name.clear();

                if (typed.is_struct()) {
                    if (typed.struct_data) {
                        temp = *typed.struct_data;
                        temp.is_assigned = true;
                    }
                    return temp;
                }

                if (typed.is_string()) {
                    temp.type = TYPE_STRING;
                    temp.str_value = typed.string_value;
                    temp.struct_type_name = type_info_to_string(TYPE_STRING);
                    temp.value = 0;
                    temp.float_value = 0.0f;
                    temp.double_value = 0.0;
                    temp.quad_value = 0.0L;
                    return temp;
                }

                TypeInfo resolved = typed.numeric_type != TYPE_UNKNOWN
                                          ? typed.numeric_type
                                          : (typed.type.type_info != TYPE_UNKNOWN
                                                 ? typed.type.type_info
                                                 : (type_hint != TYPE_UNKNOWN
                                                        ? type_hint
                                                        : TYPE_INT));

                if (resolved == TYPE_STRING && !typed.is_numeric()) {
                    temp.type = TYPE_STRING;
                    temp.str_value = typed.as_string();
                    temp.struct_type_name = type_info_to_string(TYPE_STRING);
                    return temp;
                }

                if (resolved == TYPE_FLOAT) {
                    long double quad = typed.as_quad();
                    float f = static_cast<float>(quad);
                    temp.type = TYPE_FLOAT;
                    temp.float_value = f;
                    temp.double_value = static_cast<double>(f);
                    temp.quad_value = static_cast<long double>(f);
                    temp.value = static_cast<int64_t>(f);
                } else if (resolved == TYPE_DOUBLE) {
                    long double quad = typed.as_quad();
                    double d = static_cast<double>(quad);
                    temp.type = TYPE_DOUBLE;
                    temp.float_value = static_cast<float>(d);
                    temp.double_value = d;
                    temp.quad_value = static_cast<long double>(d);
                    temp.value = static_cast<int64_t>(d);
                } else if (resolved == TYPE_QUAD) {
                    long double quad = typed.as_quad();
                    temp.type = TYPE_QUAD;
                    temp.float_value = static_cast<float>(quad);
                    temp.double_value = static_cast<double>(quad);
                    temp.quad_value = quad;
                    temp.value = static_cast<int64_t>(quad);
                } else {
                    int64_t numeric_value = typed.as_numeric();
                    if (resolved == TYPE_BOOL) {
                        numeric_value = (numeric_value != 0) ? 1 : 0;
                    }
                    temp.type = resolved;
                    temp.value = numeric_value;
                    temp.float_value = static_cast<float>(numeric_value);
                    temp.double_value = static_cast<double>(numeric_value);
                    temp.quad_value = static_cast<long double>(numeric_value);
                }

                temp.struct_type_name = type_info_to_string(temp.type);
                return temp;
            };

            const ASTNode *rhs = node->right.get();
            try {
                if (rhs->node_type == ASTNodeType::AST_VARIABLE ||
                    rhs->node_type == ASTNodeType::AST_IDENTIFIER) {
                    std::string source_var_name = rhs->name;
                    Variable *source_var =
                        interpreter_.find_variable(source_var_name);
                    if (!source_var) {
                        throw std::runtime_error(
                            "Source variable not found: " +
                            source_var_name);
                    }
                    assign_from_source(*source_var, source_var_name);
                    return;
                }

                if (rhs->node_type == ASTNodeType::AST_STRING_LITERAL) {
                    TypedValue typed_value(rhs->str_value,
                                           InferredType(TYPE_STRING,
                                                        "string"));
                    Variable temp =
                        create_temp_from_typed(typed_value, TYPE_STRING);
                    assign_from_source(temp, "");
                    return;
                }

                TypedValue typed_value =
                    interpreter_.evaluate_typed_expression(rhs);
                TypeInfo resolved_type = rhs->type_info != TYPE_UNKNOWN
                                             ? rhs->type_info
                                             : typed_value.type.type_info;
                Variable temp = create_temp_from_typed(typed_value,
                                                       resolved_type);
                assign_from_source(temp, "");
                return;
            } catch (const ReturnException &ret) {
                if (ret.is_array) {
                    throw std::runtime_error(
                        "Cannot assign array return value to interface variable '" +
                        target_name + "'");
                }

                if (!ret.is_struct) {
                    if (ret.type == TYPE_STRING) {
                        TypedValue typed_value(
                            ret.str_value,
                            InferredType(TYPE_STRING, "string"));
                        Variable temp =
                            create_temp_from_typed(typed_value,
                                                   TYPE_STRING);
                        assign_from_source(temp, "");
                        return;
                    }

                    TypeInfo resolved_type =
                        ret.type != TYPE_UNKNOWN ? ret.type : TYPE_INT;
                    TypedValue typed_value =
                        (ret.type == TYPE_FLOAT)
                            ? TypedValue(ret.double_value,
                                         InferredType(TYPE_FLOAT,
                                                      "float"))
                            : (ret.type == TYPE_DOUBLE)
                                  ? TypedValue(ret.double_value,
                                               InferredType(TYPE_DOUBLE,
                                                            "double"))
                                  : (ret.type == TYPE_QUAD)
                                        ? TypedValue(ret.quad_value,
                                                     InferredType(
                                                         TYPE_QUAD,
                                                         "quad"))
                                        : TypedValue(
                                              ret.value,
                                              InferredType(resolved_type,
                                                           type_info_to_string(
                                                               resolved_type)));
                    Variable temp = create_temp_from_typed(
                        typed_value, resolved_type);
                    assign_from_source(temp, "");
                    return;
                }

                assign_from_source(ret.struct_value, "");
                return;
            }
        }

        if (node->right && node->right->node_type == ASTNodeType::AST_FUNC_CALL) {
            try {
                TypedValue typed_value = interpreter_.evaluate_typed_expression(node->right.get());
                // TYPE_UNKNOWN をヒントとして渡し、既存の変数の型または TypedValue の型を使用
                interpreter_.assign_variable(target_name, typed_value,
                                             TYPE_UNKNOWN, false);
            } catch (const ReturnException& ret) {
                if (ret.is_struct) {
                    interpreter_.current_scope().variables[target_name] = ret.struct_value;
                    interpreter_.sync_direct_access_from_struct_value(
                        target_name,
                        interpreter_.current_scope().variables[target_name]);
                } else {
                    throw;
                }
            }
        } else {
            TypedValue typed_value = interpreter_.evaluate_typed_expression(node->right.get());
            // TYPE_UNKNOWN をヒントとして渡し、既存の変数の型または TypedValue の型を使用
            interpreter_.assign_variable(target_name, typed_value,
                                         TYPE_UNKNOWN, false);
        }
    }
}

void StatementExecutor::execute_variable_declaration(const ASTNode *node) {
    if (debug_mode) {
        std::cerr << "[DEBUG_EXEC] Executing variable declaration: " << node->name
                  << ", type_info: " << (int)node->type_info
                  << ", type_name: " << node->type_name 
                  << ", is_pointer: " << node->is_pointer
                  << ", pointer_base_type: " << (int)node->pointer_base_type
                  << ", is_reference: " << node->is_reference << std::endl;
    }
    
    // 参照型の場合の特別処理
    if (node->is_reference) {
        // 参照は必ず初期化が必要
        if (!node->init_expr && !node->right) {
            throw std::runtime_error("Reference variable '" + node->name + "' must be initialized");
        }
        
        // 初期化式を評価して参照先変数を取得
        ASTNode* init_node = node->init_expr ? node->init_expr.get() : node->right.get();
        
        // 参照先が変数でなければエラー
        if (init_node->node_type != ASTNodeType::AST_VARIABLE) {
            throw std::runtime_error("Reference variable '" + node->name + "' must be initialized with a variable");
        }
        
        std::string target_var_name = init_node->name;
        
        // 参照先変数が存在するかチェック
        Variable* target_var = interpreter_.find_variable(target_var_name);
        if (!target_var) {
            throw std::runtime_error("Reference target variable '" + target_var_name + "' not found");
        }
        
        if (debug_mode) {
            std::cerr << "[DEBUG_EXEC] Creating reference " << node->name 
                      << " -> " << target_var_name << std::endl;
        }
        
        // 参照変数を作成
        Variable ref_var;
        ref_var.is_reference = true;
        ref_var.type = target_var->type;
        ref_var.is_const = node->is_const;
        ref_var.is_array = target_var->is_array;
        ref_var.is_unsigned = target_var->is_unsigned;
        ref_var.is_struct = target_var->is_struct;
        ref_var.struct_type_name = target_var->struct_type_name;
        
        // 参照先変数のポインタを値として保存
        ref_var.value = reinterpret_cast<int64_t>(target_var);
        ref_var.is_assigned = true;
        
        if (debug_mode) {
            std::cerr << "[DEBUG_EXEC] Creating reference variable: " << node->name 
                      << ", target_value: " << target_var->value << std::endl;
        }
        
        interpreter_.current_scope().variables[node->name] = ref_var;
        return;
    }
    
    // 初期化式または右辺がある場合の特別処理
    if (node->init_expr || node->right) {
        // 初期化ノードは現在未使用だが、将来の機能拡張のため残す
        // ASTNode* init_node = node->init_expr ? node->init_expr.get() : node->right.get();
    }
    
    // Debug output removed - use --debug option if needed
    
    Variable var;
    var.type = node->type_info;
    var.is_const = node->is_const;
    var.is_array = false;
    var.is_unsigned = node->is_unsigned;

    // typedef配列の場合の特別処理
    if (node->array_type_info.base_type != TYPE_UNKNOWN) {
        // ArrayTypeInfoが設定されている場合は配列として処理
        var.is_array = true;
        var.type = node->array_type_info.base_type;
        
        // typedef名を保存（interfaceでの型マッチングに使用）
        if (!node->type_name.empty()) {
            var.struct_type_name = node->type_name;
        }
        
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
            
            // 文字列配列の場合は array_strings を初期化
            if (var.type == TYPE_STRING) {
                var.array_strings.resize(total_size, "");
                if (debug_mode) {
                    std::cerr << "DEBUG: Initialized string array with size=" << total_size << std::endl;
                }
            } else {
                var.array_values.resize(total_size, 0);
                if (debug_mode) {
                    std::cerr << "DEBUG: Initialized numeric array with size=" << total_size << std::endl;
                }
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
    
    // struct型の特別処理
    if (node->type_info == TYPE_STRUCT && !node->type_name.empty()) {
        // struct変数を作成
        std::cerr << "[DEBUG_STMT] Creating struct variable: " << node->name 
                  << " of type: " << node->type_name << std::endl;
        interpreter_.create_struct_variable(node->name, node->type_name);
        return; // struct変数は専用処理で完了
    }

    // union型の特別処理
    if (!node->type_name.empty() && interpreter_.get_type_manager()->is_union_type(node->type_name)) {
        std::cerr << "[DEBUG_STMT] Creating union variable: " << node->name 
                  << " of type: " << node->type_name << std::endl;
        
        // union型変数を作成（初期値なし）
        var.type = TYPE_UNION;
        var.type_name = node->type_name;  // union型名を保存
        interpreter_.current_scope().variables[node->name] = var;
        
        // 初期化値がある場合は検証して代入
        if (init_node) {
            execute_union_assignment(node->name, init_node);
        }
        return; // union変数は専用処理で完了
    }
    
    // 変数を現在のスコープに登録（配列リテラル代入前に必要）
    interpreter_.current_scope().variables[node->name] = var;
    
    if (init_node) {
        if (init_node->node_type == ASTNodeType::AST_TERNARY_OP) {
            // 三項演算子による初期化
            execute_ternary_variable_initialization(node, init_node);
        } else if (var.is_array && init_node->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
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
                        if (!ret.str_array_3d.empty()) {
                            // 多次元配列かどうかを判定（typedef配列名に[][]が含まれる場合）
                            bool is_multidim = (ret.array_type_name.find("[][]") != std::string::npos);
                            if (is_multidim) {
                                // 多次元配列の場合は全要素を展開
                                target_var.array_strings.clear();
                                for (const auto &plane : ret.str_array_3d) {
                                    for (const auto &row : plane) {
                                        for (const auto &element : row) {
                                            target_var.array_strings.push_back(element);
                                        }
                                    }
                                }
                                target_var.array_size = target_var.array_strings.size();
                            } else if (!ret.str_array_3d[0].empty() && 
                                      !ret.str_array_3d[0][0].empty()) {
                                // 1次元配列の場合
                                target_var.array_strings = ret.str_array_3d[0][0];
                                target_var.array_size = target_var.array_strings.size();
                            }
                            target_var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING);
                        }
                    } else if (ret.type == TYPE_FLOAT || ret.type == TYPE_DOUBLE || ret.type == TYPE_QUAD) {
                        // float/double/quad配列
                        if (!ret.double_array_3d.empty()) {
                            // 多次元配列かどうかを判定（typedef配列名に[][]が含まれる場合）
                            bool is_multidim = (ret.array_type_name.find("[][]") != std::string::npos);
                            if (is_multidim) {
                                // 多次元float/double配列の場合は全要素を展開してmultidim_array_*_valuesに設定
                                if (ret.type == TYPE_FLOAT) {
                                    target_var.multidim_array_float_values.clear();
                                    for (const auto &plane : ret.double_array_3d) {
                                        for (const auto &row : plane) {
                                            for (const auto &element : row) {
                                                target_var.multidim_array_float_values.push_back(static_cast<float>(element));
                                            }
                                        }
                                    }
                                    target_var.array_size = target_var.multidim_array_float_values.size();
                                } else if (ret.type == TYPE_DOUBLE) {
                                    target_var.multidim_array_double_values.clear();
                                    for (const auto &plane : ret.double_array_3d) {
                                        for (const auto &row : plane) {
                                            for (const auto &element : row) {
                                                target_var.multidim_array_double_values.push_back(element);
                                            }
                                        }
                                    }
                                    target_var.array_size = target_var.multidim_array_double_values.size();
                                } else { // TYPE_QUAD
                                    target_var.multidim_array_quad_values.clear();
                                    for (const auto &plane : ret.double_array_3d) {
                                        for (const auto &row : plane) {
                                            for (const auto &element : row) {
                                                target_var.multidim_array_quad_values.push_back(static_cast<long double>(element));
                                            }
                                        }
                                    }
                                    target_var.array_size = target_var.multidim_array_quad_values.size();
                                }
                                
                                // 配列の次元情報も設定（2D配列の場合）
                                target_var.is_multidimensional = true;
                                target_var.array_values.clear(); // 1次元配列はクリア
                                
                                // 2次元配列の次元情報を設定
                                if (!ret.double_array_3d.empty() && !ret.double_array_3d[0].empty()) {
                                    target_var.array_dimensions.clear();
                                    target_var.array_dimensions.push_back(ret.double_array_3d[0].size());     // 行数
                                    target_var.array_dimensions.push_back(ret.double_array_3d[0][0].size()); // 列数
                                }
                            } else if (!ret.double_array_3d[0].empty() && 
                                      !ret.double_array_3d[0][0].empty()) {
                                // 1次元float/double配列の場合
                                if (ret.type == TYPE_FLOAT) {
                                    target_var.array_float_values.clear();
                                    for (const auto &element : ret.double_array_3d[0][0]) {
                                        target_var.array_float_values.push_back(static_cast<float>(element));
                                    }
                                    target_var.array_size = target_var.array_float_values.size();
                                } else if (ret.type == TYPE_DOUBLE) {
                                    target_var.array_double_values.clear();
                                    for (const auto &element : ret.double_array_3d[0][0]) {
                                        target_var.array_double_values.push_back(element);
                                    }
                                    target_var.array_size = target_var.array_double_values.size();
                                } else { // TYPE_QUAD
                                    target_var.array_quad_values.clear();
                                    for (const auto &element : ret.double_array_3d[0][0]) {
                                        target_var.array_quad_values.push_back(static_cast<long double>(element));
                                    }
                                    target_var.array_size = target_var.array_quad_values.size();
                                }
                            }
                            target_var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + ret.type);
                        }
                    } else {
                        // 整数型配列
                        if (!ret.int_array_3d.empty()) {
                            // 多次元配列かどうかを判定（typedef配列名に[][]が含まれる場合）
                            bool is_multidim = (ret.array_type_name.find("[][]") != std::string::npos);
                            if (is_multidim) {
                                // 多次元配列の場合は全要素を展開してmultidim_array_valuesに設定
                                target_var.multidim_array_values.clear();
                                for (const auto &plane : ret.int_array_3d) {
                                    for (const auto &row : plane) {
                                        for (const auto &element : row) {
                                            target_var.multidim_array_values.push_back(element);
                                        }
                                    }
                                }
                                // 配列の次元情報も設定（2D配列の場合）
                                target_var.is_multidimensional = true;
                                target_var.array_size = target_var.multidim_array_values.size();
                                target_var.array_values.clear(); // 1次元配列はクリア
                                
                                // 2次元配列の次元情報を設定
                                if (!ret.int_array_3d.empty() && !ret.int_array_3d[0].empty()) {
                                    target_var.array_dimensions.clear();
                                    target_var.array_dimensions.push_back(ret.int_array_3d[0].size());     // 行数
                                    target_var.array_dimensions.push_back(ret.int_array_3d[0][0].size()); // 列数
                                }
                            } else if (!ret.int_array_3d[0].empty() && 
                                      !ret.int_array_3d[0][0].empty()) {
                                // 1次元配列の場合
                                target_var.array_values = ret.int_array_3d[0][0];
                                target_var.array_size = target_var.array_values.size();
                            }
                            target_var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + ret.type);
                        }
                    }
                    target_var.is_assigned = true;
                } else {
                    // 非配列戻り値の場合
                    if (ret.is_struct) {
                        // 構造体戻り値の場合
                        // printf("STRUCT_VAR_DECL_DEBUG: Assigning struct return to variable %s\n", node->name.c_str());
                        
                        // 変数を構造体型に設定
                        Variable& target_var = interpreter_.current_scope().variables[node->name];
                        target_var = ret.struct_value;  // 構造体をコピー
                        target_var.is_assigned = true;
                        
                        // 個別メンバー変数も作成
                        for (const auto& member : ret.struct_value.struct_members) {
                            std::string member_path = node->name + "." + member.first;
                            interpreter_.current_scope().variables[member_path] = member.second;
                        }
                    } else if (ret.type == TYPE_STRING) {
                        interpreter_.current_scope().variables[node->name].str_value = ret.str_value;
                    } else {
                        // 数値戻り値（float/double/quad対応）
                        if (ret.type == TYPE_FLOAT) {
                            InferredType float_type(TYPE_FLOAT, "float");
                            TypedValue typed_val(ret.double_value, float_type);
                            interpreter_.assign_variable(node->name, typed_val, ret.type, false);
                        } else if (ret.type == TYPE_DOUBLE) {
                            InferredType double_type(TYPE_DOUBLE, "double");
                            TypedValue typed_val(ret.double_value, double_type);
                            interpreter_.assign_variable(node->name, typed_val, ret.type, false);
                        } else if (ret.type == TYPE_QUAD) {
                            InferredType quad_type(TYPE_QUAD, "quad");
                            TypedValue typed_val(ret.quad_value, quad_type);
                            interpreter_.assign_variable(node->name, typed_val, ret.type, false);
                        } else {
                            interpreter_.assign_variable(node->name, ret.value, ret.type);
                        }
                    }
                    interpreter_.current_scope().variables[node->name].is_assigned = true;
                }
            }
        } else {
            // 通常の初期化 - TypedValue を使用して float/double を保持
            if (init_node->node_type == ASTNodeType::AST_FUNC_CALL) {
                try {
                    TypedValue typed_value = interpreter_.evaluate_typed(init_node);
                    if (var.type == TYPE_STRING && !typed_value.is_string()) {
                        // 文字列型なのに数値が返された場合
                        throw std::runtime_error("Type mismatch: expected string but got numeric value");
                    } else {
                        interpreter_.assign_variable(node->name, typed_value,
                                                     node->type_info, false);
                    }
                    interpreter_.current_scope().variables[node->name].is_assigned = true;
                } catch (const ReturnException& ret) {
                    if (ret.is_struct) {
                        // 構造体戻り値の場合
                        printf("STRUCT_INIT_DEBUG: Assigning struct return to variable %s\n", node->name.c_str());
                        
                        Variable& target_var = interpreter_.current_scope().variables[node->name];
                        target_var = ret.struct_value;  // 構造体をコピー
                        target_var.is_assigned = true;
                        
                        // 個別メンバー変数も作成
                        for (const auto& member : ret.struct_value.struct_members) {
                            std::string member_path = node->name + "." + member.first;
                            interpreter_.current_scope().variables[member_path] = member.second;
                        }
                    } else if (ret.type == TYPE_STRING) {
                        interpreter_.current_scope().variables[node->name].str_value = ret.str_value;
                        interpreter_.current_scope().variables[node->name].type = TYPE_STRING;
                    } else {
                        // 数値戻り値（float/double/quad対応）
                        if (ret.type == TYPE_FLOAT) {
                            InferredType float_type(TYPE_FLOAT, "float");
                            TypedValue typed_val(ret.double_value, float_type);
                            interpreter_.assign_variable(node->name, typed_val, ret.type, false);
                        } else if (ret.type == TYPE_DOUBLE) {
                            InferredType double_type(TYPE_DOUBLE, "double");
                            TypedValue typed_val(ret.double_value, double_type);
                            interpreter_.assign_variable(node->name, typed_val, ret.type, false);
                        } else if (ret.type == TYPE_QUAD) {
                            InferredType quad_type(TYPE_QUAD, "quad");
                            TypedValue typed_val(ret.quad_value, quad_type);
                            interpreter_.assign_variable(node->name, typed_val, ret.type, false);
                        } else {
                            interpreter_.assign_variable(node->name, ret.value, ret.type);
                        }
                    }
                    interpreter_.current_scope().variables[node->name].is_assigned = true;
                }
            } else {
                // float/double リテラルを含む全ての初期化式で TypedValue を使用
                TypedValue typed_value = interpreter_.evaluate_typed(init_node);
                
                if (interpreter_.is_debug_mode() && node->name == "ptr") {
                    std::cerr << "[STMT_EXEC] Initializing variable ptr:" << std::endl;
                    std::cerr << "  node->type_info=" << static_cast<int>(node->type_info) << std::endl;
                    std::cerr << "  TYPE_STRING=" << static_cast<int>(TYPE_STRING) << std::endl;
                    std::cerr << "  TYPE_POINTER=" << static_cast<int>(TYPE_POINTER) << std::endl;
                    std::cerr << "  var.type=" << static_cast<int>(var.type) << std::endl;
                }
                
                if (var.type == TYPE_STRING) {
                    interpreter_.current_scope().variables[node->name].str_value = init_node->str_value;
                } else if (node->type_info == TYPE_POINTER) {
                    // ポインタ型は精度損失を避けるため、直接valueフィールドに代入
                    interpreter_.current_scope().variables[node->name].value = typed_value.value;
                    interpreter_.current_scope().variables[node->name].type = TYPE_POINTER;
                    
                    if (interpreter_.is_debug_mode()) {
                        std::cerr << "[STMT_EXEC] Pointer variable " << node->name << " initialized with value="
                                  << typed_value.value << " (0x" << std::hex << typed_value.value << std::dec << ")" << std::endl;
                    }
                } else {
                    interpreter_.assign_variable(node->name, typed_value,
                                                 node->type_info, false);
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
    
    // 初期化式がある場合の処理
    if (node->init_expr) {
        if (node->type_info == TYPE_STRUCT && node->init_expr->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
            // struct配列リテラル初期化: Person[3] people = [{25, "Alice"}, {30, "Bob"}];
            execute_struct_array_literal_init(node->name, node->init_expr.get(), node->type_name);
        } else if (node->init_expr->node_type == ASTNodeType::AST_FUNC_CALL) {
            // 配列を返す関数呼び出し: double[2][3] arr = make_array();
            try {
                int64_t value = interpreter_.evaluate(node->init_expr.get());
                // void関数の場合
                interpreter_.current_scope().variables[node->name].value = value;
                interpreter_.current_scope().variables[node->name].is_assigned = true;
            } catch (const ReturnException& ret) {
                if (ret.is_array) {
                    // 配列戻り値の場合
                    Variable& target_var = interpreter_.current_scope().variables[node->name];
                    
                    if (ret.type == TYPE_STRING) {
                        // 文字列配列
                        if (!ret.str_array_3d.empty()) {
                            bool is_multidim = (ret.array_type_name.find("[][]") != std::string::npos);
                            if (is_multidim) {
                                target_var.array_strings.clear();
                                for (const auto &plane : ret.str_array_3d) {
                                    for (const auto &row : plane) {
                                        for (const auto &element : row) {
                                            target_var.array_strings.push_back(element);
                                        }
                                    }
                                }
                                target_var.array_size = target_var.array_strings.size();
                            } else if (!ret.str_array_3d[0].empty() && !ret.str_array_3d[0][0].empty()) {
                                target_var.array_strings = ret.str_array_3d[0][0];
                                target_var.array_size = target_var.array_strings.size();
                            }
                            target_var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING);
                        }
                    } else if (ret.type == TYPE_FLOAT || ret.type == TYPE_DOUBLE || ret.type == TYPE_QUAD) {
                        // float/double/quad配列
                        if (!ret.double_array_3d.empty()) {
                            bool is_multidim = (ret.array_type_name.find("[][]") != std::string::npos ||
                                               ret.double_array_3d.size() > 1 ||
                                               (ret.double_array_3d.size() == 1 && ret.double_array_3d[0].size() > 1));
                            if (is_multidim) {
                                // 多次元float/double配列の場合
                                if (ret.type == TYPE_FLOAT) {
                                    target_var.multidim_array_float_values.clear();
                                    for (const auto &plane : ret.double_array_3d) {
                                        for (const auto &row : plane) {
                                            for (const auto &element : row) {
                                                target_var.multidim_array_float_values.push_back(static_cast<float>(element));
                                            }
                                        }
                                    }
                                    target_var.array_size = target_var.multidim_array_float_values.size();
                                } else if (ret.type == TYPE_DOUBLE) {
                                    target_var.multidim_array_double_values.clear();
                                    for (const auto &plane : ret.double_array_3d) {
                                        for (const auto &row : plane) {
                                            for (const auto &element : row) {
                                                target_var.multidim_array_double_values.push_back(element);
                                            }
                                        }
                                    }
                                    target_var.array_size = target_var.multidim_array_double_values.size();
                                } else { // TYPE_QUAD
                                    target_var.multidim_array_quad_values.clear();
                                    for (const auto &plane : ret.double_array_3d) {
                                        for (const auto &row : plane) {
                                            for (const auto &element : row) {
                                                target_var.multidim_array_quad_values.push_back(static_cast<long double>(element));
                                            }
                                        }
                                    }
                                    target_var.array_size = target_var.multidim_array_quad_values.size();
                                }
                                target_var.is_multidimensional = true;
                                target_var.array_values.clear();
                                
                                // 2次元配列の次元情報を設定
                                if (!ret.double_array_3d.empty() && !ret.double_array_3d[0].empty()) {
                                    target_var.array_dimensions.clear();
                                    target_var.array_dimensions.push_back(ret.double_array_3d[0].size());
                                    target_var.array_dimensions.push_back(ret.double_array_3d[0][0].size());
                                }
                            } else if (!ret.double_array_3d[0].empty() && !ret.double_array_3d[0][0].empty()) {
                                // 1次元float/double配列の場合
                                if (ret.type == TYPE_FLOAT) {
                                    target_var.array_float_values.clear();
                                    for (const auto &element : ret.double_array_3d[0][0]) {
                                        target_var.array_float_values.push_back(static_cast<float>(element));
                                    }
                                    target_var.array_size = target_var.array_float_values.size();
                                } else if (ret.type == TYPE_DOUBLE) {
                                    target_var.array_double_values.clear();
                                    for (const auto &element : ret.double_array_3d[0][0]) {
                                        target_var.array_double_values.push_back(element);
                                    }
                                    target_var.array_size = target_var.array_double_values.size();
                                } else { // TYPE_QUAD
                                    target_var.array_quad_values.clear();
                                    for (const auto &element : ret.double_array_3d[0][0]) {
                                        target_var.array_quad_values.push_back(static_cast<long double>(element));
                                    }
                                    target_var.array_size = target_var.array_quad_values.size();
                                }
                            }
                            target_var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + ret.type);
                        }
                    } else {
                        // 整数型配列
                        if (!ret.int_array_3d.empty()) {
                            bool is_multidim = (ret.array_type_name.find("[][]") != std::string::npos ||
                                               ret.int_array_3d.size() > 1 ||
                                               (ret.int_array_3d.size() == 1 && ret.int_array_3d[0].size() > 1));
                            if (is_multidim) {
                                target_var.multidim_array_values.clear();
                                for (const auto &plane : ret.int_array_3d) {
                                    for (const auto &row : plane) {
                                        for (const auto &element : row) {
                                            target_var.multidim_array_values.push_back(element);
                                        }
                                    }
                                }
                                target_var.array_size = target_var.multidim_array_values.size();
                                target_var.is_multidimensional = true;
                                target_var.array_values.clear();
                                
                                if (!ret.int_array_3d.empty() && !ret.int_array_3d[0].empty()) {
                                    target_var.array_dimensions.clear();
                                    target_var.array_dimensions.push_back(ret.int_array_3d[0].size());
                                    target_var.array_dimensions.push_back(ret.int_array_3d[0][0].size());
                                }
                            } else if (!ret.int_array_3d[0].empty() && !ret.int_array_3d[0][0].empty()) {
                                target_var.array_values = ret.int_array_3d[0][0];
                                target_var.array_size = target_var.array_values.size();
                            }
                            target_var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + ret.type);
                        }
                    }
                    target_var.is_assigned = true;
                }
            }
        }
        // 他の配列初期化は既存の処理で対応
    }
}

void StatementExecutor::execute_struct_array_literal_init(const std::string& array_name, const ASTNode* array_literal, const std::string& struct_type) {
    if (!array_literal || array_literal->node_type != ASTNodeType::AST_ARRAY_LITERAL) {
        throw std::runtime_error("Invalid array literal for struct array initialization");
    }
    
    // 各配列要素（struct literal）を処理
    for (size_t i = 0; i < array_literal->arguments.size(); i++) {
        const ASTNode* struct_literal = array_literal->arguments[i].get();
        if (struct_literal->node_type != ASTNodeType::AST_STRUCT_LITERAL) {
            throw std::runtime_error("Expected struct literal in struct array initialization");
        }
        
        std::string element_name = array_name + "[" + std::to_string(i) + "]";
        interpreter_.assign_struct_literal(element_name, struct_literal);
    }
}

void StatementExecutor::execute_member_array_assignment(const ASTNode* node) {
    debug_print("DEBUG: execute_member_array_assignment called\n");
    // obj.member[index] = value の処理
    const ASTNode* member_array_access = node->left.get();
    
    if (!member_array_access || member_array_access->node_type != ASTNodeType::AST_MEMBER_ARRAY_ACCESS) {
        debug_print("DEBUG: Not AST_MEMBER_ARRAY_ACCESS, node_type=%d\n", 
                   member_array_access ? static_cast<int>(member_array_access->node_type) : -1);
        throw std::runtime_error("Invalid member array access in assignment");
    }
    
    // オブジェクト名を取得
    std::string obj_name;
    std::string array_member_name;  // obj.array[idx].member の "array" 部分
    bool is_nested_struct_array_access = false;
    
    if (member_array_access->left && 
        (member_array_access->left->node_type == ASTNodeType::AST_VARIABLE ||
         member_array_access->left->node_type == ASTNodeType::AST_IDENTIFIER)) {
        obj_name = member_array_access->left->name;
    } else if (member_array_access->left && 
               member_array_access->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
        // 2つのケースをチェック:
        // 1. s.grades[0] = 85 (構造体メンバーの配列へのアクセス)
        // 2. triangle.points[0].x = 1 (構造体配列メンバーの要素へのメンバーアクセス)
        
        if (member_array_access->left->left &&
            (member_array_access->left->left->node_type == ASTNodeType::AST_VARIABLE ||
             member_array_access->left->left->node_type == ASTNodeType::AST_IDENTIFIER)) {
            obj_name = member_array_access->left->left->name;
            array_member_name = member_array_access->left->name;
            
            // member_array_access->name が設定されている場合、これは obj.array[idx].member のパターン
            if (!member_array_access->name.empty() && 
                member_array_access->name != array_member_name) {
                is_nested_struct_array_access = true;
                debug_print("DEBUG: Detected nested struct array member access: %s.%s[idx].%s\n", 
                           obj_name.c_str(), array_member_name.c_str(), member_array_access->name.c_str());
            }
        } else {
            debug_print("ERROR: Nested member_array_access->left->left->node_type = %d\n", 
                       member_array_access->left->left ? static_cast<int>(member_array_access->left->left->node_type) : -1);
            throw std::runtime_error("Invalid nested object reference in member array access");
        }
    } else {
        if (member_array_access->left) {
            debug_print("ERROR: member_array_access->left->node_type = %d\n", 
                       static_cast<int>(member_array_access->left->node_type));
        } else {
            debug_print("ERROR: member_array_access->left is null\n");
        }
        throw std::runtime_error("Invalid object reference in member array access");
    }
    
    // メンバ名を取得
    std::string member_name;
    if (is_nested_struct_array_access) {
        // triangle.points[0].x = 1 の場合
        member_name = member_array_access->name;  // "x"
    } else if (member_array_access->left && 
        member_array_access->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
        // ネストされた場合: s.grades[0] の "grades" 部分
        member_name = member_array_access->left->name;
    } else {
        // 直接の場合
        member_name = member_array_access->name;
    }
    
    debug_print("DEBUG: obj_name='%s', member_name='%s', array_member='%s', is_nested=%d\n", 
                obj_name.c_str(), member_name.c_str(), array_member_name.c_str(), is_nested_struct_array_access);
    
    // インデックス値を評価（多次元対応）
    std::vector<int64_t> indices;
    if (member_array_access->right) {
        // 1次元の場合（従来通り）
        int64_t index = interpreter_.evaluate(member_array_access->right.get());
        indices.push_back(index);
    } else if (!member_array_access->arguments.empty()) {
        // 多次元の場合
        for (const auto& arg : member_array_access->arguments) {
            int64_t index = interpreter_.evaluate(arg.get());
            indices.push_back(index);
        }
    } else {
        throw std::runtime_error("No indices found for array access in member array assignment");
    }
    
    // ネストされた構造体配列メンバーアクセスの処理: obj.array[idx].member = value
    if (is_nested_struct_array_access) {
        debug_print("DEBUG: Processing nested struct array member assignment\n");
        int array_index = static_cast<int>(indices[0]);
        
        // array_member_name の配列から要素を取得
        Variable* array_member = interpreter_.get_struct_member(obj_name, array_member_name);
        if (!array_member) {
            throw std::runtime_error("Struct member not found: " + array_member_name);
        }
        
        if (!array_member->is_array) {
            throw std::runtime_error("Member is not an array: " + array_member_name);
        }
        
        // 配列インデックスの境界チェック
        if (array_index < 0 || array_index >= array_member->array_size) {
            throw std::runtime_error("Array index out of bounds: " + std::to_string(array_index));
        }
        
        // struct_members内で配列要素にアクセス
        // 構造体配列の要素は "array_member_name[index]" という名前で個別に格納されている
        // まず array_member の struct_members を調べる
        std::string element_key = array_member_name + "[" + std::to_string(array_index) + "]";
        
        debug_print("DEBUG: Looking for struct array element: %s\n", element_key.c_str());
        
        // 親構造体から配列要素を探す（最初に親のstruct_membersを確認）
        Variable* parent_struct = interpreter_.find_variable(obj_name);
        if (!parent_struct || !parent_struct->is_struct) {
            throw std::runtime_error("Parent variable is not a struct: " + obj_name);
        }
        
        // 配列要素の構造体を探す - まず親のstruct_membersから
        auto element_it = parent_struct->struct_members.find(element_key);
        if (element_it == parent_struct->struct_members.end()) {
            // 配列メンバー自体のstruct_membersから探す
            element_it = array_member->struct_members.find(element_key);
            if (element_it == array_member->struct_members.end()) {
                debug_print("DEBUG: Available keys in parent struct_members:\n");
                for (const auto& pair : parent_struct->struct_members) {
                    debug_print("  - %s\n", pair.first.c_str());
                }
                debug_print("DEBUG: Available keys in array_member struct_members:\n");
                for (const auto& pair : array_member->struct_members) {
                    debug_print("  - %s\n", pair.first.c_str());
                }
                throw std::runtime_error("Struct array element not found: " + element_key);
            }
        }
        
        Variable& struct_element = element_it->second;
        debug_print("DEBUG: Found struct array element, is_struct=%s, struct_members.size()=%zu\n",
                   struct_element.is_struct ? "true" : "false",
                   struct_element.struct_members.size());
        if (!struct_element.is_struct) {
            throw std::runtime_error("Array element is not a struct");
        }
        
        // 構造体要素のメンバーに値を代入
        auto member_it = struct_element.struct_members.find(member_name);
        if (member_it == struct_element.struct_members.end()) {
            throw std::runtime_error("Struct member not found in array element: " + member_name);
        }
        
        // 右辺の値を評価して代入
        if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
            member_it->second.str_value = node->right->str_value;
            member_it->second.type = TYPE_STRING;
            debug_print("DEBUG_ASSIGN: Assigned string '%s' to %s.%s[%d].%s\n", 
                       node->right->str_value.c_str(), obj_name.c_str(), 
                       array_member_name.c_str(), array_index, member_name.c_str());
        } else {
            TypedValue typed_value = interpreter_.evaluate_typed(node->right.get());
            if (typed_value.is_floating()) {
                member_it->second.double_value = typed_value.as_double();
                member_it->second.type = typed_value.type.type_info;
                debug_print("DEBUG_ASSIGN: Assigned double %f to %s.%s[%d].%s\n", 
                           typed_value.as_double(), obj_name.c_str(), 
                           array_member_name.c_str(), array_index, member_name.c_str());
            } else {
                int64_t value = typed_value.as_numeric();
                member_it->second.value = value;
                member_it->second.type = typed_value.type.type_info;
                debug_print("DEBUG_ASSIGN: Assigned integer %lld to %s.%s[%d].%s\n", 
                           value, obj_name.c_str(), 
                           array_member_name.c_str(), array_index, member_name.c_str());
            }
        }
        member_it->second.is_assigned = true;
        
        // ダイレクトアクセス変数も更新
        std::string direct_access_name = obj_name + "." + element_key + "." + member_name;
        Variable* direct_var = interpreter_.find_variable(direct_access_name);
        if (direct_var) {
            if (member_it->second.type == TYPE_STRING) {
                direct_var->str_value = member_it->second.str_value;
            } else if (member_it->second.type == TYPE_FLOAT || member_it->second.type == TYPE_DOUBLE) {
                direct_var->double_value = member_it->second.double_value;
            } else {
                direct_var->value = member_it->second.value;
            }
            direct_var->type = member_it->second.type;
            direct_var->is_assigned = true;
            debug_print("DEBUG_ASSIGN: Updated direct access variable: %s = %lld\n", 
                       direct_access_name.c_str(), direct_var->value);
        }
        
        // 構造体配列要素変数自体の struct_members も更新
        std::string element_var_name = obj_name + "." + element_key;
        Variable* element_variable = interpreter_.find_variable(element_var_name);
        if (element_variable && element_variable->is_struct) {
            auto elem_member_it = element_variable->struct_members.find(member_name);
            if (elem_member_it != element_variable->struct_members.end()) {
                elem_member_it->second = member_it->second;
                debug_print("DEBUG_ASSIGN: Updated element variable struct_members: %s.%s = %lld\n", 
                           element_var_name.c_str(), member_name.c_str(), elem_member_it->second.value);
            }
        }
        
        debug_print("DEBUG: Nested struct array member assigned: %s.%s[%d].%s\n", 
                   obj_name.c_str(), array_member_name.c_str(), array_index, member_name.c_str());
        return;
    }
    
    // 右辺の値を評価して構造体メンバー配列要素に代入
    debug_print("DEBUG: execute_member_array_assignment - right type=%d, indices count=%zu\n", 
                static_cast<int>(node->right->node_type), indices.size());
                
    if (indices.size() > 1) {
        // 多次元配列の場合
        Variable* member_var = interpreter_.get_struct_member(obj_name, member_name);
        if (!member_var) {
            throw std::runtime_error("Struct member not found: " + member_name);
        }
        
        if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
            interpreter_.setMultidimensionalStringArrayElement(*member_var, indices, node->right->str_value);
        } else {
            int64_t value = interpreter_.evaluate(node->right.get());
            interpreter_.setMultidimensionalArrayElement(*member_var, indices, value);
        }
        return; // 多次元処理完了
    }
    
    // 1次元配列の場合（従来処理）
    int index = static_cast<int>(indices[0]);
    if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
        interpreter_.assign_struct_member_array_element(obj_name, member_name, index, node->right->str_value);
    } else if (node->right->node_type == ASTNodeType::AST_ARRAY_REF) {
        // 構造体メンバ配列アクセスがAST_ARRAY_REFとして解析される場合
        debug_print("DEBUG: Processing AST_ARRAY_REF on right-hand side in array assignment\n");
        if (node->right->left && node->right->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
            // original.tags[0] の形式
            std::string right_obj_name = node->right->left->left->name;
            std::string right_member_name = node->right->left->name;
            int64_t array_index = interpreter_.evaluate(node->right->array_index.get());
            
            Variable* right_member_var = interpreter_.get_struct_member(right_obj_name, right_member_name);
            debug_print("DEBUG: AST_ARRAY_REF right_member_var type=%d, is_array=%d\n", 
                       static_cast<int>(right_member_var->type), right_member_var->is_array ? 1 : 0);
            if ((right_member_var->type == TYPE_STRING && right_member_var->is_array) || 
                right_member_var->type == static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING)) {
                debug_print("DEBUG: Using string array element access via AST_ARRAY_REF\n");
                std::string str_value = interpreter_.get_struct_member_array_string_element(right_obj_name, right_member_name, static_cast<int>(array_index));
                interpreter_.assign_struct_member_array_element(obj_name, member_name, index, str_value);
            } else {
                debug_print("DEBUG: Using numeric array element access via AST_ARRAY_REF\n");
                int64_t value = interpreter_.get_struct_member_array_element(right_obj_name, right_member_name, static_cast<int>(array_index));
                interpreter_.assign_struct_member_array_element(obj_name, member_name, index, value);
            }
        } else {
            // 通常の配列参照として処理
            int64_t value = interpreter_.evaluate(node->right.get());
            interpreter_.assign_struct_member_array_element(obj_name, member_name, index, value);
        }
    } else if (node->right->node_type == ASTNodeType::AST_MEMBER_ARRAY_ACCESS) {
        // 構造体メンバ配列アクセスの場合（original.tags[0]等）
        debug_print("DEBUG: Processing AST_MEMBER_ARRAY_ACCESS on right-hand side in array assignment\n");
        std::string right_obj_name;
        std::string right_member_name = node->right->name;
        
        if (node->right->left->node_type == ASTNodeType::AST_VARIABLE) {
            right_obj_name = node->right->left->name;
        } else if (node->right->left->node_type == ASTNodeType::AST_ARRAY_REF) {
            // struct配列要素の場合
            std::string array_name = node->right->left->left->name;
            int64_t idx = interpreter_.evaluate(node->right->left->array_index.get());
            right_obj_name = array_name + "[" + std::to_string(idx) + "]";
        } else {
            throw std::runtime_error("Invalid right-hand member array access");
        }
        
        // インデックスを評価
        int64_t array_index = interpreter_.evaluate(node->right->right.get());
        
        // 右辺の構造体メンバ配列要素を取得
        Variable* right_member_var = interpreter_.get_struct_member(right_obj_name, right_member_name);
        debug_print("DEBUG: right_member_var type=%d, is_array=%d in array assignment\n", 
                   static_cast<int>(right_member_var->type), right_member_var->is_array ? 1 : 0);
        if ((right_member_var->type == TYPE_STRING && right_member_var->is_array) || 
            right_member_var->type == static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING)) {
            debug_print("DEBUG: Using string array element access in array assignment\n");
            std::string str_value = interpreter_.get_struct_member_array_string_element(right_obj_name, right_member_name, static_cast<int>(array_index));
            interpreter_.assign_struct_member_array_element(obj_name, member_name, index, str_value);
        } else {
            debug_print("DEBUG: Using numeric array element access in array assignment\n");
            int64_t value = interpreter_.get_struct_member_array_element(right_obj_name, right_member_name, static_cast<int>(array_index));
            interpreter_.assign_struct_member_array_element(obj_name, member_name, index, value);
        }
    } else {
        int64_t value = interpreter_.evaluate(node->right.get());
        interpreter_.assign_struct_member_array_element(obj_name, member_name, index, value);
    }
}

void StatementExecutor::execute_member_assignment(const ASTNode* node) {
    // obj.member = value または array[index].member = value の処理
    const ASTNode* member_access = node->left.get();
    
    if (debug_mode) {
        debug_print("DEBUG: execute_member_assignment - starting\n");
    }
    debug_print("DEBUG: execute_member_assignment - left type=%d, right type=%d\n",
               static_cast<int>(node->left->node_type), static_cast<int>(node->right->node_type));
    
    if (member_access->left) {
        debug_print("DEBUG: member_access->left->node_type=%d, name='%s'\n", 
                   static_cast<int>(member_access->left->node_type), 
                   member_access->left->name.c_str());
    } else {
        debug_print("DEBUG: member_access->left is null\n");
    }
    
    if (!member_access || member_access->node_type != ASTNodeType::AST_MEMBER_ACCESS) {
        throw std::runtime_error("Invalid member access in assignment");
    }
    
    // オブジェクト名を取得
    std::string obj_name;
    if (member_access->left && (member_access->left->node_type == ASTNodeType::AST_VARIABLE || 
                               member_access->left->node_type == ASTNodeType::AST_IDENTIFIER)) {
        // 通常の構造体変数: obj.member または self.member
        obj_name = member_access->left->name;
        
        // selfの場合は特別処理
        if (obj_name == "self") {
            debug_msg(DebugMsgId::SELF_MEMBER_ACCESS_START, member_access->name.c_str());
            // selfへの代入処理を実行
            execute_self_member_assignment(member_access->name, node->right.get());
            return;
        }
        
        if (debug_mode) {
            debug_print("DEBUG: Struct member access - variable: %s\n", obj_name.c_str());
        }
    } else if (member_access->left && member_access->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
        // ネストメンバアクセス: obj.mid.data.value = 100
        // member_accessの左側のメンバアクセスチェーンを評価して、最後の構造体変数を取得
        debug_print("DEBUG: Nested member access assignment - member=%s\n", member_access->name.c_str());
        
        // ルート変数名を取得
        const ASTNode* root_node = member_access->left.get();
        while (root_node && root_node->node_type == ASTNodeType::AST_MEMBER_ACCESS && root_node->left) {
            root_node = root_node->left.get();
        }
        std::string root_var_name;
        if (root_node && (root_node->node_type == ASTNodeType::AST_VARIABLE || root_node->node_type == ASTNodeType::AST_IDENTIFIER)) {
            root_var_name = root_node->name;
        }
        
        // ルート変数がconstかチェック
        if (!root_var_name.empty()) {
            Variable* root_var = interpreter_.find_variable(root_var_name);
            if (root_var && root_var->is_const) {
                throw std::runtime_error("Cannot assign to member of const struct: " + root_var_name);
            }
        }
        
        // ネストメンバアクセスを評価して対象の構造体変数を取得
        Variable* target_struct = evaluate_nested_member_access(member_access->left.get());
        
        if (!target_struct) {
            throw std::runtime_error("Cannot resolve nested member access");
        }
        
        // 左側のメンバアクセスのメンバ名を取得
        std::string parent_member = member_access->left->name;
        debug_print("DEBUG: Parent member: %s\n", parent_member.c_str());
        
        // parent_memberが構造体メンバかどうか確認
        auto parent_it = target_struct->struct_members.find(parent_member);
        if (parent_it == target_struct->struct_members.end()) {
            throw std::runtime_error("Parent member not found: " + parent_member);
        }
        
        Variable& parent_member_var = parent_it->second;
        if (parent_member_var.type != TYPE_STRUCT) {
            throw std::runtime_error("Parent member is not a struct: " + parent_member);
        }
        
        // 最終的なメンバ名を取得
        std::string member_name = member_access->name;
        debug_print("DEBUG: Final member: %s\n", member_name.c_str());
        
        // constメンバへの代入チェック
        auto final_member_it = parent_member_var.struct_members.find(member_name);
        if (final_member_it != parent_member_var.struct_members.end()) {
            debug_print("DEBUG: Nested member const check: %s.%s - is_const=%d, is_assigned=%d\n",
                       parent_member.c_str(), member_name.c_str(),
                       final_member_it->second.is_const ? 1 : 0,
                       final_member_it->second.is_assigned ? 1 : 0);
            if (final_member_it->second.is_const && final_member_it->second.is_assigned) {
                throw std::runtime_error("Cannot assign to const member '" + member_name + 
                                       "' after initialization");
            }
        }
        
        //  完全なパスを構築
        std::function<std::string(const ASTNode*)> build_full_path;
        build_full_path = [&](const ASTNode* n) -> std::string {
            if (!n) return "";
            if (n->node_type == ASTNodeType::AST_VARIABLE || n->node_type == ASTNodeType::AST_IDENTIFIER) {
                return n->name;
            } else if (n->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
                std::string base = build_full_path(n->left.get());
                return base.empty() ? n->name : base + "." + n->name;
            }
            return "";
        };
        std::string full_member_path = build_full_path(member_access);
        
        if (debug_mode) {
            debug_print("DEBUG: Nested member assignment - full_path='%s'\n", full_member_path.c_str());
        }
        
        // 完全パスで個別変数を直接更新
        if (!full_member_path.empty()) {
            Variable* individual_var = interpreter_.find_variable(full_member_path);
            if (individual_var) {
                if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
                    individual_var->str_value = node->right->str_value;
                    individual_var->type = TYPE_STRING;
                } else {
                    TypedValue typed_value = interpreter_.evaluate_typed(node->right.get());
                    individual_var->value = typed_value.as_numeric();
                    individual_var->type = typed_value.type.type_info;
                }
                individual_var->is_assigned = true;
                
                if (debug_mode) {
                    debug_print("DEBUG: Updated individual variable '%s' = %lld\n", 
                               full_member_path.c_str(), individual_var->value);
                }
            } else {
                if (debug_mode) {
                    debug_print("DEBUG: Individual variable '%s' not found!\n", full_member_path.c_str());
                }
            }
        }
        
        // struct_members階層も更新（互換性のため）
        if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
            parent_member_var.struct_members[member_name].str_value = node->right->str_value;
            parent_member_var.struct_members[member_name].type = TYPE_STRING;
        } else {
            TypedValue typed_value = interpreter_.evaluate_typed(node->right.get());
            parent_member_var.struct_members[member_name].value = typed_value.as_numeric();
            parent_member_var.struct_members[member_name].type = typed_value.type.type_info;
        }
        parent_member_var.struct_members[member_name].is_assigned = true;
        
        return;
    } else if (member_access->left && member_access->left->node_type == ASTNodeType::AST_ARRAY_REF) {
        // 構造体配列要素のメンバ: struct.array[index].member
        // または単純な配列要素: array[index].member
        
        const ASTNode* array_ref = member_access->left.get();
        
        // 配列参照の左側を評価して完全な名前を構築
        std::string array_base_name;
        if (array_ref->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
            // struct.array[index].member の場合
            std::string struct_name = array_ref->left->left->name;
            std::string array_member = array_ref->left->name;
            array_base_name = struct_name + "." + array_member;
        } else {
            // array[index].member の場合
            array_base_name = array_ref->left->name;
        }
        
        int64_t index = interpreter_.evaluate(array_ref->array_index.get());
        obj_name = array_base_name + "[" + std::to_string(index) + "]";
        
        if (debug_mode) {
            debug_print("DEBUG: Struct array element member assignment: %s.%s\n", 
                       obj_name.c_str(), member_access->name.c_str());
        }
    } else if (member_access->left && member_access->left->node_type == ASTNodeType::AST_UNARY_OP && 
               member_access->left->op == "DEREFERENCE") {
        // デリファレンスされたポインタ: (*pp).member
        debug_print("DEBUG: Dereference pointer member assignment\n");
        
        // ポインタ変数を評価（デリファレンスの左側）
        int64_t ptr_value = interpreter_.evaluate(member_access->left->left.get());
        Variable* struct_var = reinterpret_cast<Variable*>(ptr_value);
        
        if (!struct_var) {
            throw std::runtime_error("Null pointer dereference in member assignment");
        }
        
        // メンバ名を取得
        std::string member_name = member_access->name;
        
        // 右辺を評価
        if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
            struct_var->struct_members[member_name].str_value = node->right->str_value;
            struct_var->struct_members[member_name].type = TYPE_STRING;
        } else {
            TypedValue typed_value = interpreter_.evaluate_typed(node->right.get());
            struct_var->struct_members[member_name].value = typed_value.as_numeric();
            struct_var->struct_members[member_name].type = typed_value.type.type_info;
        }
        struct_var->struct_members[member_name].is_assigned = true;
        
        return;
    } else {
        throw std::runtime_error("Invalid object reference in member access");
    }
    
    // メンバ名を取得
    std::string member_name = member_access->name;
    
    // constメンバへの代入チェック
    Variable* target_var = interpreter_.find_variable(obj_name);
    if (target_var && target_var->is_struct) {
        auto member_it = target_var->struct_members.find(member_name);
        if (member_it != target_var->struct_members.end()) {
            if (member_it->second.is_const && member_it->second.is_assigned) {
                throw std::runtime_error("Cannot assign to const member '" + member_name + 
                                       "' of struct '" + obj_name + "' after initialization");
            }
        }
    }
    
    // ネストしたメンバーの場合、最上位の親変数のconstもチェック
    std::string root_obj_name = obj_name;
    size_t dot_pos = obj_name.find('.');
    if (dot_pos != std::string::npos) {
        root_obj_name = obj_name.substr(0, dot_pos);
        if (Variable *root_var = interpreter_.find_variable(root_obj_name)) {
            if (root_var->is_const) {
                throw std::runtime_error("Cannot assign to member of const struct: " +
                                       obj_name + "." + member_name);
            }
        }
    }
    
    // struct変数のメンバに直接代入
    if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
        interpreter_.assign_struct_member(obj_name, member_name, node->right->str_value);
    } else if (node->right->node_type == ASTNodeType::AST_VARIABLE) {
        // 変数参照の場合、構造体変数か数値/文字列変数か判断
        Variable* right_var = interpreter_.find_variable(node->right->name);
        
        if (!right_var) {
            throw std::runtime_error("Right-hand variable not found: " + node->right->name);
        }
        
        // 構造体変数の場合、ReturnExceptionをキャッチして構造体を代入
        if (right_var->type == TYPE_STRUCT) {
            try {
                // 構造体変数を評価（ReturnExceptionが投げられる）
                interpreter_.evaluate(node->right.get());
                throw std::runtime_error("Expected struct variable to throw ReturnException");
            } catch (const ReturnException& ret_ex) {
                if (ret_ex.struct_value.type == TYPE_STRUCT) {
                    std::cerr << "DEBUG: Assigning struct to member: " << obj_name << "." << member_name << std::endl;
                    std::cerr << "DEBUG: Source struct type: " << ret_ex.struct_value.struct_type_name << std::endl;
                    
                    // 構造体全体をメンバーに代入
                    interpreter_.assign_struct_member_struct(obj_name, member_name, ret_ex.struct_value);
                } else {
                    throw std::runtime_error("Variable is not a struct for struct member assignment");
                }
            }
        } else if (right_var->type == TYPE_STRING) {
            interpreter_.assign_struct_member(obj_name, member_name, right_var->str_value);
        } else {
            // TypedValueを使用して型情報を保持
            TypedValue typed_value = interpreter_.evaluate_typed(node->right.get());
            interpreter_.assign_struct_member(obj_name, member_name, typed_value);
        }
    } else if (node->right->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
        // 構造体メンバアクセスの場合（original.name等）
        std::string right_obj_name;
        std::string right_member_name = node->right->name;
        
        if (node->right->left->node_type == ASTNodeType::AST_VARIABLE) {
            right_obj_name = node->right->left->name;
        } else if (node->right->left->node_type == ASTNodeType::AST_ARRAY_REF) {
            // struct配列要素の場合
            std::string array_name = node->right->left->left->name;
            int64_t index = interpreter_.evaluate(node->right->left->array_index.get());
            right_obj_name = array_name + "[" + std::to_string(index) + "]";
        } else {
            throw std::runtime_error("Invalid right-hand member access");
        }
        
        // 右辺の構造体メンバを取得
        Variable* right_member_var = interpreter_.get_struct_member(right_obj_name, right_member_name);
        if (right_member_var->type == TYPE_STRING) {
            interpreter_.assign_struct_member(obj_name, member_name, right_member_var->str_value);
        } else if (right_member_var->type == TYPE_FLOAT || right_member_var->type == TYPE_DOUBLE || right_member_var->type == TYPE_QUAD) {
            // 浮動小数点型の場合はTypedValueを作成
            InferredType inferred;
            inferred.type_info = right_member_var->type;
            if (right_member_var->type == TYPE_FLOAT) {
                TypedValue typed_value(static_cast<double>(right_member_var->float_value), inferred);
                typed_value.numeric_type = TYPE_FLOAT;
                interpreter_.assign_struct_member(obj_name, member_name, typed_value);
            } else if (right_member_var->type == TYPE_DOUBLE) {
                TypedValue typed_value(right_member_var->double_value, inferred);
                typed_value.numeric_type = TYPE_DOUBLE;
                interpreter_.assign_struct_member(obj_name, member_name, typed_value);
            } else {
                TypedValue typed_value(right_member_var->quad_value, inferred);
                typed_value.numeric_type = TYPE_QUAD;
                interpreter_.assign_struct_member(obj_name, member_name, typed_value);
            }
        } else {
            interpreter_.assign_struct_member(obj_name, member_name, right_member_var->value);
        }
    } else if (node->right->node_type == ASTNodeType::AST_MEMBER_ARRAY_ACCESS) {
        // 構造体メンバ配列アクセスの場合（original.tags[0]等）
        debug_print("DEBUG: Processing AST_MEMBER_ARRAY_ACCESS on right-hand side\n");
        std::string right_obj_name;
        std::string right_member_name = node->right->name;
        
        if (node->right->left->node_type == ASTNodeType::AST_VARIABLE) {
            right_obj_name = node->right->left->name;
        } else if (node->right->left->node_type == ASTNodeType::AST_ARRAY_REF) {
            // struct配列要素の場合
            std::string array_name = node->right->left->left->name;
            int64_t index = interpreter_.evaluate(node->right->left->array_index.get());
            right_obj_name = array_name + "[" + std::to_string(index) + "]";
        } else {
            throw std::runtime_error("Invalid right-hand member array access");
        }
        
        // インデックスを評価
        int64_t array_index = interpreter_.evaluate(node->right->right.get());
        
        // 右辺の構造体メンバ配列要素を取得
        Variable* right_member_var = interpreter_.get_struct_member(right_obj_name, right_member_name);
        debug_print("DEBUG: right_member_var type=%d, is_array=%d\n", 
                   static_cast<int>(right_member_var->type), right_member_var->is_array ? 1 : 0);
        if ((right_member_var->type == TYPE_STRING && right_member_var->is_array) || 
            right_member_var->type == static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING)) {
            debug_print("DEBUG: Using string array element access\n");
            std::string str_value = interpreter_.get_struct_member_array_string_element(right_obj_name, right_member_name, static_cast<int>(array_index));
            interpreter_.assign_struct_member(obj_name, member_name, str_value);
        } else {
            debug_print("DEBUG: Using numeric array element access\n");
            int64_t value = interpreter_.get_struct_member_array_element(right_obj_name, right_member_name, static_cast<int>(array_index));
            interpreter_.assign_struct_member(obj_name, member_name, value);
        }
    } else {
        // TypedValueを使用して型情報を保持
        TypedValue typed_value = interpreter_.evaluate_typed(node->right.get());
        interpreter_.assign_struct_member(obj_name, member_name, typed_value);
    }
}

void StatementExecutor::execute_arrow_assignment(const ASTNode* node) {
    // ptr->member = value の処理（アロー演算子は (*ptr).member と等価）
    const ASTNode* arrow_access = node->left.get();
    
    if (debug_mode) {
        debug_print("DEBUG: execute_arrow_assignment - starting\n");
    }
    debug_print("DEBUG: execute_arrow_assignment - left type=%d, right type=%d\n",
               static_cast<int>(node->left->node_type), static_cast<int>(node->right->node_type));
    
    if (!arrow_access || arrow_access->node_type != ASTNodeType::AST_ARROW_ACCESS) {
        throw std::runtime_error("Invalid arrow access in assignment");
    }
    
    // ポインタを評価
    int64_t ptr_value = interpreter_.evaluate(arrow_access->left.get());
    
    if (ptr_value == 0) {
        throw std::runtime_error("Null pointer dereference in arrow assignment");
    }
    
    // ポインタから構造体変数を取得
    Variable* struct_var = reinterpret_cast<Variable*>(ptr_value);
    
    if (!struct_var) {
        throw std::runtime_error("Invalid pointer in arrow assignment");
    }
    
    // メンバ名を取得
    std::string member_name = arrow_access->name;
    
    // 右辺を評価して代入
    if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
        struct_var->struct_members[member_name].str_value = node->right->str_value;
        struct_var->struct_members[member_name].type = TYPE_STRING;
    } else {
        TypedValue typed_value = interpreter_.evaluate_typed(node->right.get());
        struct_var->struct_members[member_name].value = typed_value.as_numeric();
        struct_var->struct_members[member_name].type = typed_value.type.type_info;
    }
    struct_var->struct_members[member_name].is_assigned = true;
    
    if (debug_mode) {
        debug_print("DEBUG: execute_arrow_assignment - completed\n");
    }
}

void StatementExecutor::execute_member_array_literal_assignment(const ASTNode* node) {
    // obj.member = [1, 2, 3] または array[index].member = [1, 2, 3] の処理
    const ASTNode* member_access = node->left.get();
    
    if (!member_access || member_access->node_type != ASTNodeType::AST_MEMBER_ACCESS) {
        throw std::runtime_error("Invalid member access in array literal assignment");
    }
    
    // オブジェクト名を取得
    std::string obj_name;
    if (member_access->left && member_access->left->node_type == ASTNodeType::AST_VARIABLE) {
        // 通常の構造体変数: obj.member
        obj_name = member_access->left->name;
    } else if (member_access->left && member_access->left->node_type == ASTNodeType::AST_ARRAY_REF) {
        // 構造体配列要素: array[index].member
        std::string array_name = member_access->left->left->name;
        int64_t index = interpreter_.evaluate(member_access->left->array_index.get());
        obj_name = array_name + "[" + std::to_string(index) + "]";
    } else {
        throw std::runtime_error("Invalid object reference in member array literal assignment");
    }
    
    // メンバ名を取得
    std::string member_name = member_access->name;
    
    if (debug_mode) {
        std::cerr << "DEBUG: Member array literal assignment: " << obj_name << "." << member_name << std::endl;
    }
    
    // 構造体メンバー配列への配列リテラル代入
    interpreter_.assign_struct_member_array_literal(obj_name, member_name, node->right.get());
}

void StatementExecutor::execute_union_assignment(const std::string& var_name, const ASTNode* value_node) {
    // union型変数への代入を実行
    auto& var = interpreter_.current_scope().variables[var_name];
    
    if (var.type != TYPE_UNION) {
        throw std::runtime_error("Variable is not a union type: " + var_name);
    }
    
    std::string union_type_name = var.type_name;
    
    // 値の型に応じて検証と代入を実行
    if (value_node->node_type == ASTNodeType::AST_STRING_LITERAL) {
        // 文字列値
        std::string str_value = value_node->str_value;
        if (interpreter_.get_type_manager()->is_value_allowed_for_union(union_type_name, str_value)) {
            var.str_value = str_value;
            var.current_type = TYPE_STRING;
        } else {
            throw std::runtime_error("String value '" + str_value + "' is not allowed for union type " + union_type_name);
        }
    } else if (value_node->node_type == ASTNodeType::AST_NUMBER) {
        // 数値
        int64_t int_value = value_node->int_value;
        if (interpreter_.get_type_manager()->is_value_allowed_for_union(union_type_name, int_value)) {
            var.value = int_value;
            var.current_type = TYPE_INT;
        } else {
            throw std::runtime_error("Integer value " + std::to_string(int_value) + " is not allowed for union type " + union_type_name);
        }
    } else {
        // 式の評価
        try {
            // まず文字列として評価してみる
            if (value_node->node_type == ASTNodeType::AST_VARIABLE) {
                // 変数参照の場合、変数の値を取得
                auto& source_var = interpreter_.current_scope().variables[value_node->name];
                if (source_var.current_type == TYPE_STRING) {
                    if (interpreter_.get_type_manager()->is_value_allowed_for_union(union_type_name, source_var.str_value)) {
                        var.str_value = source_var.str_value;
                        var.current_type = TYPE_STRING;
                        return;
                    }
                } else {
                    int64_t int_value = source_var.value;
                    if (interpreter_.get_type_manager()->is_value_allowed_for_union(union_type_name, int_value)) {
                        var.value = int_value;
                        var.current_type = TYPE_INT;
                        return;
                    }
                }
            }
            
            // 数値として評価
            int64_t int_value = interpreter_.evaluate(value_node);
            if (interpreter_.get_type_manager()->is_value_allowed_for_union(union_type_name, int_value)) {
                var.value = int_value;
                var.current_type = TYPE_INT;
            } else {
                throw std::runtime_error("Value " + std::to_string(int_value) + " is not allowed for union type " + union_type_name);
            }
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to assign value to union variable " + var_name + ": " + e.what());
        }
    }
}

void StatementExecutor::execute_self_member_assignment(const std::string& member_name, const ASTNode* value_node) {
    debug_msg(DebugMsgId::SELF_MEMBER_ACCESS_START, member_name.c_str());
    
    // selfメンバーへのパスを構築
    std::string self_member_path = "self." + member_name;
    
    // selfメンバー変数を検索
    Variable* self_member = interpreter_.find_variable(self_member_path);
    if (!self_member) {
        throw std::runtime_error("Self member not found: " + member_name);
    }
    
    debug_msg(DebugMsgId::SELF_MEMBER_ACCESS_FOUND, member_name.c_str());
    
    // 元のレシーバー変数からselfメンバーのパスを取得
    Variable* self_var = interpreter_.find_variable("self");
    Variable* receiver_info = interpreter_.find_variable("__self_receiver__");
    std::string original_receiver_path;
    
    if (self_var && receiver_info && !receiver_info->str_value.empty()) {
        original_receiver_path = receiver_info->str_value + "." + member_name;
        debug_print("SELF_ASSIGN_DEBUG: Original receiver path: %s\n", original_receiver_path.c_str());
    }
    
    // 値の型に応じて代入処理
    if (value_node->node_type == ASTNodeType::AST_STRING_LITERAL) {
        self_member->str_value = value_node->str_value;
        self_member->type = TYPE_STRING;
        self_member->is_assigned = true;
        
        // 元の変数のメンバーも同時に更新
        if (!original_receiver_path.empty()) {
            debug_print("SELF_ASSIGN_DEBUG: Looking for original member: %s\n", original_receiver_path.c_str());
            Variable* original_member = interpreter_.find_variable(original_receiver_path);
            if (original_member) {
                debug_print("SELF_ASSIGN_DEBUG: Found original member, updating string value\n");
                original_member->str_value = value_node->str_value;
                original_member->type = TYPE_STRING;
                original_member->is_assigned = true;
                debug_print("SELF_ASSIGN_SYNC: %s = \"%s\"\n", original_receiver_path.c_str(), value_node->str_value.c_str());
            } else {
                debug_print("SELF_ASSIGN_DEBUG: Could not find original member: %s\n", original_receiver_path.c_str());
            }
        }
        
        debug_print("SELF_ASSIGN: %s = \"%s\"\n", member_name.c_str(), value_node->str_value.c_str());
    } else if (value_node->node_type == ASTNodeType::AST_VARIABLE) {
        // 変数参照の場合
        Variable* source_var = interpreter_.find_variable(value_node->name);
        if (source_var && source_var->type == TYPE_STRING) {
            self_member->str_value = source_var->str_value;
            self_member->type = TYPE_STRING;
            
            // 元の変数のメンバーも同時に更新
            if (!original_receiver_path.empty()) {
                debug_print("SELF_ASSIGN_DEBUG: Looking for original member: %s\n", original_receiver_path.c_str());
                Variable* original_member = interpreter_.find_variable(original_receiver_path);
                if (original_member) {
                    debug_print("SELF_ASSIGN_DEBUG: Found original member, updating string value from variable\n");
                    original_member->str_value = source_var->str_value;
                    original_member->type = TYPE_STRING;
                    original_member->is_assigned = true;
                    debug_print("SELF_ASSIGN_SYNC: %s = \"%s\" (from variable)\n", original_receiver_path.c_str(), source_var->str_value.c_str());
                } else {
                    debug_print("SELF_ASSIGN_DEBUG: Could not find original member: %s\n", original_receiver_path.c_str());
                }
            }
            
            debug_print("SELF_ASSIGN: %s = \"%s\" (from variable)\n", member_name.c_str(), source_var->str_value.c_str());
        } else {
            int64_t value = interpreter_.evaluate(value_node);
            self_member->value = value;
            if (self_member->type != TYPE_STRING) {
                self_member->type = TYPE_INT; // デフォルトはint型
            }
            
            // 元の変数のメンバーも同時に更新
            if (!original_receiver_path.empty()) {
                debug_print("SELF_ASSIGN_DEBUG: Looking for original member: %s\n", original_receiver_path.c_str());
                Variable* original_member = interpreter_.find_variable(original_receiver_path);
                if (original_member) {
                    debug_print("SELF_ASSIGN_DEBUG: Found original member, updating numeric value from variable\n");
                    original_member->value = value;
                    if (original_member->type != TYPE_STRING) {
                        original_member->type = TYPE_INT;
                    }
                    original_member->is_assigned = true;
                    debug_print("SELF_ASSIGN_SYNC: %s = %lld (from variable)\n", original_receiver_path.c_str(), (long long)value);
                } else {
                    debug_print("SELF_ASSIGN_DEBUG: Could not find original member: %s\n", original_receiver_path.c_str());
                }
            }
            
            debug_print("SELF_ASSIGN: %s = %lld (from variable)\n", member_name.c_str(), (long long)value);
        }
        self_member->is_assigned = true;
    } else {
        // 式の評価
        int64_t value = interpreter_.evaluate(value_node);
        
        // 複合代入演算子の処理
        if (value_node->node_type == ASTNodeType::AST_BINARY_OP) {
            // += -= *= /= などの複合代入かチェック
            if (value_node->name == "+=" || value_node->name == "-=" || 
                value_node->name == "*=" || value_node->name == "/=") {
                // 複合代入は既に評価済みの値として処理
                debug_print("SELF_COMPOUND_ASSIGN: %s %s= %lld\n", 
                          member_name.c_str(), value_node->name.c_str(), (long long)value);
            }
        }
        
        self_member->value = value;
        if (self_member->type != TYPE_STRING) {
            self_member->type = TYPE_INT;
        }
        self_member->is_assigned = true;
        
        // 元の変数のメンバーも同時に更新
        if (!original_receiver_path.empty()) {
            debug_print("SELF_ASSIGN_DEBUG: Looking for original member: %s\n", original_receiver_path.c_str());
            Variable* original_member = interpreter_.find_variable(original_receiver_path);
            if (original_member) {
                debug_print("SELF_ASSIGN_DEBUG: Found original member, updating numeric value\n");
                original_member->value = value;
                if (original_member->type != TYPE_STRING) {
                    original_member->type = TYPE_INT;
                }
                original_member->is_assigned = true;
                debug_print("SELF_ASSIGN_SYNC: %s = %lld\n", original_receiver_path.c_str(), (long long)value);
            } else {
                debug_print("SELF_ASSIGN_DEBUG: Could not find original member: %s\n", original_receiver_path.c_str());
            }
        }
        
        debug_print("SELF_ASSIGN: %s = %lld\n", member_name.c_str(), (long long)value);
    }
    
    std::string self_value_str = std::to_string(self_member->value);
    debug_msg(DebugMsgId::SELF_MEMBER_ACCESS_VALUE, self_value_str.c_str());
}

void StatementExecutor::execute_ternary_assignment(const ASTNode *node) {
    // 三項演算子の条件を評価
    int64_t condition = interpreter_.evaluate(node->right->left.get());
    
    // 条件に基づいて選択される分岐を決定
    const ASTNode* selected_branch = condition ? node->right->right.get() : node->right->third.get();
    
    // 選択された分岐の型に基づいて処理を分岐
    if (selected_branch->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
        // 配列リテラルの代入
        if (!node->name.empty()) {
            interpreter_.assign_array_literal(node->name, selected_branch);
            return;
        }
    } else if (selected_branch->node_type == ASTNodeType::AST_STRUCT_LITERAL) {
        // 構造体リテラルの代入
        if (!node->name.empty()) {
            interpreter_.assign_struct_literal(node->name, selected_branch);
            return;
        }
    } else if (selected_branch->node_type == ASTNodeType::AST_STRING_LITERAL) {
        // 文字列リテラルの代入
        if (!node->name.empty()) {
            Variable* var = interpreter_.get_variable(node->name);
            if (var) {
                var->str_value = selected_branch->str_value;
                var->type = TYPE_STRING;
                var->is_assigned = true;
            }
        }
        return;
    } else {
        // その他（数値、関数呼び出しなど）の場合は通常の評価
        try {
            TypedValue typed_value = interpreter_.evaluate_typed_expression(selected_branch);
            interpreter_.assign_variable(node->name, typed_value,
                                         typed_value.type.type_info,
                                         false);
        } catch (const ReturnException& ret) {
            if (!node->name.empty()) {
                if (ret.type == TYPE_STRING) {
                    TypedValue typed_value(ret.str_value,
                                           InferredType(TYPE_STRING, "string"));
                    interpreter_.assign_variable(node->name, typed_value,
                                                 TYPE_STRING, false);
                } else if (ret.type == TYPE_FLOAT || ret.type == TYPE_DOUBLE ||
                           ret.type == TYPE_QUAD) {
                    TypeInfo numeric_type = ret.type;
                    long double quad_value =
                        (ret.type == TYPE_FLOAT)
                            ? static_cast<long double>(ret.double_value)
                            : (ret.type == TYPE_DOUBLE)
                                  ? static_cast<long double>(ret.double_value)
                                  : ret.quad_value;
                    TypedValue typed_value(quad_value,
                                           InferredType(numeric_type,
                                                        type_info_to_string(numeric_type)));
                    interpreter_.assign_variable(node->name, typed_value,
                                                 numeric_type, false);
                } else if (ret.is_struct) {
                    Variable struct_var = ret.struct_value;
                    TypedValue typed_value(struct_var,
                                           InferredType(TYPE_STRUCT,
                                                        struct_var.struct_type_name));
                    interpreter_.assign_variable(node->name, typed_value,
                                                 TYPE_STRUCT, false);
                } else {
                    TypedValue typed_value(ret.value,
                                           InferredType(ret.type,
                                                        type_info_to_string(ret.type)));
                    interpreter_.assign_variable(node->name, typed_value,
                                                 ret.type, false);
                }
            }
        }
    }
}

void StatementExecutor::execute_ternary_variable_initialization(const ASTNode* var_decl_node, const ASTNode* ternary_node) {
    printf("DEBUG: execute_ternary_variable_initialization called\n");
    
    // 三項演算子の条件を評価
    int64_t condition = interpreter_.evaluate(ternary_node->left.get());
    printf("DEBUG: Ternary condition = %lld\n", condition);
    
    // 条件に基づいて選択される分岐を決定
    const ASTNode* selected_branch = condition ? ternary_node->right.get() : ternary_node->third.get();
    printf("DEBUG: Selected branch node_type = %d\n", static_cast<int>(selected_branch->node_type));
    
    std::string var_name = var_decl_node->name;
    Variable* var = interpreter_.get_variable(var_name);
    
    if (!var) {
        throw std::runtime_error("Variable not found during ternary initialization: " + var_name);
    }
    
    // 選択された分岐の型に基づいて処理
    if (selected_branch->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
        // 配列リテラルの初期化
        interpreter_.assign_array_literal(var_name, selected_branch);
        var->is_assigned = true;
    } else if (selected_branch->node_type == ASTNodeType::AST_STRUCT_LITERAL) {
        // 構造体リテラルの初期化
        interpreter_.assign_struct_literal(var_name, selected_branch);
        var->is_assigned = true;
    } else if (selected_branch->node_type == ASTNodeType::AST_STRING_LITERAL) {
        // 文字列リテラルの初期化
        var->str_value = selected_branch->str_value;
        var->type = TYPE_STRING;
        var->is_assigned = true;
    } else {
        try {
            TypedValue typed_value = interpreter_.evaluate_typed_expression(selected_branch);
            interpreter_.assign_variable(var_name, typed_value,
                                         typed_value.type.type_info,
                                         false);
        } catch (const ReturnException& ret) {
            if (ret.type == TYPE_STRING) {
                TypedValue typed_value(ret.str_value,
                                       InferredType(TYPE_STRING, "string"));
                interpreter_.assign_variable(var_name, typed_value,
                                             TYPE_STRING, false);
            } else if (ret.type == TYPE_FLOAT || ret.type == TYPE_DOUBLE ||
                       ret.type == TYPE_QUAD) {
                TypeInfo numeric_type = ret.type;
                long double quad_value =
                    (ret.type == TYPE_FLOAT)
                        ? static_cast<long double>(ret.double_value)
                        : (ret.type == TYPE_DOUBLE)
                              ? static_cast<long double>(ret.double_value)
                              : ret.quad_value;
                TypedValue typed_value(quad_value,
                                       InferredType(numeric_type,
                                                    type_info_to_string(numeric_type)));
                interpreter_.assign_variable(var_name, typed_value,
                                             numeric_type, false);
            } else if (ret.is_struct) {
                Variable struct_var = ret.struct_value;
                TypedValue typed_value(struct_var,
                                       InferredType(TYPE_STRUCT,
                                                    struct_var.struct_type_name));
                interpreter_.assign_variable(var_name, typed_value,
                                             TYPE_STRUCT, false);
            } else {
                TypedValue typed_value(ret.value,
                                       InferredType(ret.type,
                                                    type_info_to_string(ret.type)));
                interpreter_.assign_variable(var_name, typed_value,
                                             ret.type, false);
            }
        }
    }
}

Variable* StatementExecutor::evaluate_nested_member_access(const ASTNode* member_access_node) {
    // ネストメンバアクセス (obj.mid.data) を再帰的に評価して、最終的なメンバを含む親構造体を返す
    if (!member_access_node || member_access_node->node_type != ASTNodeType::AST_MEMBER_ACCESS) {
        return nullptr;
    }
    
    // 左側を取得
    if (!member_access_node->left) {
        return nullptr;
    }
    
    Variable* parent_struct = nullptr;
    
    if (member_access_node->left->node_type == ASTNodeType::AST_VARIABLE ||
        member_access_node->left->node_type == ASTNodeType::AST_IDENTIFIER) {
        // 基底オブジェクト: obj または self
        std::string obj_name = member_access_node->left->name;
        parent_struct = interpreter_.find_variable(obj_name);
        
        if (!parent_struct || parent_struct->type != TYPE_STRUCT) {
            throw std::runtime_error("Base object is not a struct: " + obj_name);
        }
    } else if (member_access_node->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
        // ネストメンバアクセス: obj.mid (さらに再帰)
        // 左側のメンバアクセスを評価して、その親構造体を取得
        Variable* intermediate_struct = evaluate_nested_member_access(member_access_node->left.get());
        if (!intermediate_struct) {
            return nullptr;
        }
        
        // 左側のメンバ名を取得
        std::string intermediate_member = member_access_node->left->name;
        
        // 親構造体から中間メンバを取得
        auto it = intermediate_struct->struct_members.find(intermediate_member);
        if (it == intermediate_struct->struct_members.end()) {
            throw std::runtime_error("Intermediate member not found: " + intermediate_member);
        }
        
        parent_struct = &it->second;
        if (parent_struct->type != TYPE_STRUCT) {
            throw std::runtime_error("Intermediate member is not a struct: " + intermediate_member);
        }
    } else {
        throw std::runtime_error("Unsupported nested member access left node type");
    }
    
    return parent_struct;
}
