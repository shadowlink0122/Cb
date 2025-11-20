// v0.14.0: HIR to C++ Transpiler - Expression Generation
// 式生成モジュール

#include "hir_to_cpp.h"
#include "../../common/debug.h"
#include <algorithm>

namespace cb {
namespace codegen {

using namespace ir::hir;

std::string HIRToCpp::generate_expr(const HIRExpr &expr) {
    switch (expr.kind) {
    case HIRExpr::ExprKind::Literal:
        return generate_literal(expr);
    case HIRExpr::ExprKind::Variable:
        return generate_variable(expr);
    case HIRExpr::ExprKind::BinaryOp:
        return generate_binary_op(expr);
    case HIRExpr::ExprKind::UnaryOp:
        return generate_unary_op(expr);
    case HIRExpr::ExprKind::FunctionCall:
        return generate_function_call(expr);
    case HIRExpr::ExprKind::MethodCall:
        return generate_method_call(expr);
    case HIRExpr::ExprKind::MemberAccess:
        return generate_member_access(expr);
    case HIRExpr::ExprKind::ArrayAccess:
        return generate_array_access(expr);
    case HIRExpr::ExprKind::Cast:
        return generate_cast(expr);
    case HIRExpr::ExprKind::Ternary:
        return generate_ternary(expr);
    case HIRExpr::ExprKind::Lambda:
        return generate_lambda(expr);
    case HIRExpr::ExprKind::StructLiteral:
        return generate_struct_literal(expr);
    case HIRExpr::ExprKind::ArrayLiteral:
        return generate_array_literal(expr);
    case HIRExpr::ExprKind::AddressOf:
        return generate_address_of(expr);
    case HIRExpr::ExprKind::Dereference:
        return generate_dereference(expr);
    case HIRExpr::ExprKind::SizeOf:
        return generate_sizeof(expr);
    case HIRExpr::ExprKind::New:
        return generate_new(expr);
    case HIRExpr::ExprKind::Await:
        return generate_await(expr);
    case HIRExpr::ExprKind::PreIncDec:
        return generate_pre_incdec(expr);
    case HIRExpr::ExprKind::PostIncDec:
        return generate_post_incdec(expr);
    default:
        return "/* unsupported expr */";
    }
}

std::string HIRToCpp::generate_literal(const HIRExpr &expr) {
    // nullptr
    if (expr.literal_type.kind == HIRType::TypeKind::Nullptr) {
        debug_msg(DebugMsgId::CODEGEN_CPP_POINTER_NULL);
        return "nullptr";
    }
    // 文字列リテラルはエスケープ処理
    if (expr.literal_type.kind == HIRType::TypeKind::String) {
        return "\"" + escape_string(expr.literal_value) + "\"";
    }
    return expr.literal_value;
}

std::string HIRToCpp::generate_variable(const HIRExpr &expr) {
    // Convert 'self' to appropriate representation
    if (expr.var_name == "self") {
        if (current_impl_is_for_primitive) {
            // For primitive type impl Model specialization, use 'data'
            return "data";
        } else {
            // For struct impls, use (*this)
            return "(*this)";
        }
    }
    return add_hir_prefix(expr.var_name);
}

std::string HIRToCpp::generate_binary_op(const HIRExpr &expr) {
    // Helper lambda to check if a type name is a union
    auto is_union_type = [this](const std::string &type_name) -> bool {
        if (!current_program || type_name.empty()) return false;
        for (const auto &union_def : current_program->unions) {
            if (union_def.name == type_name) {
                return true;
            }
        }
        return false;
    };

    // Helper lambda to get type name for an expression
    auto get_expr_type_name = [this](const HIRExpr *e) -> std::string {
        if (!e || !current_program) return "";

        // Check direct type info
        if (!e->type.name.empty()) {
            return e->type.name;
        }

        // For MemberAccess, look up the struct field type
        if (e->kind == HIRExpr::ExprKind::MemberAccess && e->object) {
            // Get the object's struct type
            std::string struct_name;

            // Check object's type name
            if (!e->object->type.name.empty()) {
                struct_name = e->object->type.name;
            }
            // For Variable, look up from function parameters
            else if (e->object->kind == HIRExpr::ExprKind::Variable) {
                auto it = current_function_params.find(e->object->var_name);
                if (it != current_function_params.end()) {
                    struct_name = it->second.name;
                }
            }

            // Look up the struct and find the field type
            if (!struct_name.empty()) {
                for (const auto &s : current_program->structs) {
                    if (s.name == struct_name) {
                        for (const auto &field : s.fields) {
                            if (field.name == e->member_name) {
                                return field.type.name;
                            }
                        }
                        break;
                    }
                }
            }

            // Fallback: search all structs for the field name
            // This handles local variables where type info is not available
            if (!e->member_name.empty()) {
                for (const auto &s : current_program->structs) {
                    for (const auto &field : s.fields) {
                        if (field.name == e->member_name && !field.type.name.empty()) {
                            // Check if this field type is a union
                            for (const auto &union_def : current_program->unions) {
                                if (union_def.name == field.type.name) {
                                    return field.type.name;
                                }
                            }
                        }
                    }
                }
            }
        }

        return "";
    };

    // Union型の算術演算の特殊処理
    // 左辺がUnion型で算術演算の場合、std::get<int>()でラップ
    if (expr.left && expr.right && current_program) {
        std::string left_type_name = get_expr_type_name(expr.left.get());
        std::string right_type_name = get_expr_type_name(expr.right.get());

        bool left_is_union = is_union_type(left_type_name);
        bool right_is_union = is_union_type(right_type_name);

        // 算術演算子かチェック
        bool is_arithmetic = (expr.op == "+" || expr.op == "-" ||
                              expr.op == "*" || expr.op == "/" || expr.op == "%");

        if ((left_is_union || right_is_union) && is_arithmetic) {
            std::string result = "(";
            if (left_is_union) {
                result += "std::get<int>(" + generate_expr(*expr.left) + ")";
            } else {
                result += generate_expr(*expr.left);
            }
            result += " " + expr.op + " ";
            if (right_is_union) {
                result += "std::get<int>(" + generate_expr(*expr.right) + ")";
            } else {
                result += generate_expr(*expr.right);
            }
            result += ")";
            return result;
        }
    }

    // ポインタ演算の特殊処理: + or - は全てchar*経由で行う（void*対応のため）
    // 型情報が不完全な場合があるため、全ての+/-に適用
    // ポインタ演算の特別処理
    if ((expr.op == "+" || expr.op == "-") && expr.left && expr.right) {
        // 型情報ベースの判定を最優先
        bool is_pointer_arithmetic = false;

        // 文字列連結を除外: 左辺が文字列リテラル、または右辺が文字列型の場合
        bool is_string_concat = false;
        if (expr.left->kind == HIRExpr::ExprKind::Literal &&
            expr.left->type.kind == HIRType::TypeKind::String) {
            is_string_concat = true;
        }
        // 右辺が関数呼び出し(to_string_helper等)または文字列型の場合も文字列連結
        if (expr.right->kind == HIRExpr::ExprKind::FunctionCall ||
            expr.right->type.kind == HIRType::TypeKind::String) {
            is_string_concat = true;
        }

        if (is_string_concat) {
            // 文字列連結は通常のBinaryOpとして処理
            std::string result = "(";
            result += generate_expr(*expr.left);
            result += " " + expr.op + " ";
            result += generate_expr(*expr.right);
            result += ")";
            return result;
        }

        // 1. 左辺の型情報が利用可能な場合
        if (expr.left->type.kind == HIRType::TypeKind::Pointer) {
            is_pointer_arithmetic = true;
        }
        // 2. MemberAccessの場合、型をチェック
        else if (expr.left->kind == HIRExpr::ExprKind::MemberAccess) {
            if (expr.left->type.kind == HIRType::TypeKind::Pointer) {
                is_pointer_arithmetic = true;
            }
        }

        // 3. 型情報が不十分な場合、名前ベースのヒューリスティック
        if (!is_pointer_arithmetic) {
            std::string left_expr_str = generate_expr(*expr.left);

            // 関数呼び出しを最初にチェック（最優先で除外）
            // 括弧があればほぼ関数呼び出し（malloc以外）
            bool has_paren = left_expr_str.find("(") != std::string::npos;
            bool is_malloc = left_expr_str.find("malloc") != std::string::npos;
            bool is_explicit_cast =
                left_expr_str.find("(void*)") != std::string::npos ||
                left_expr_str.find("(char*)") != std::string::npos ||
                left_expr_str.find("(int)") != std::string::npos;

            if (has_paren && !is_malloc && !is_explicit_cast) {
                // 関数呼び出しまたは(*this)などの式
                // ただし、.front/.backは明確にポインタメンバー
                if (left_expr_str.find(".front") != std::string::npos ||
                    left_expr_str.find(".back") != std::string::npos) {
                    is_pointer_arithmetic = true;
                } else {
                    // 関数呼び出しの結果は整数として扱う
                    is_pointer_arithmetic = false;
                }
            }
            // mallocやキャストは明確なポインタ
            else if (is_malloc || is_explicit_cast) {
                is_pointer_arithmetic = true;
            }
            // _ptr, _node, _array などの命名規則
            else if (left_expr_str.find("CB_HIR_current") !=
                     std::string::npos) {
                is_pointer_arithmetic = true;
            } else if ((left_expr_str.find("_ptr") != std::string::npos &&
                        left_expr_str.find("ptr_size") == std::string::npos) ||
                       (left_expr_str.find("_node") != std::string::npos &&
                        left_expr_str.find("node_count") ==
                            std::string::npos) ||
                       (left_expr_str.find("_array") != std::string::npos &&
                        left_expr_str.find("array_size") ==
                            std::string::npos)) {
                is_pointer_arithmetic = true;
            }
            // .front, .back などのポインタメンバー（括弧なしの場合）
            else if (left_expr_str.find(".front") != std::string::npos ||
                     left_expr_str.find(".back") != std::string::npos) {
                is_pointer_arithmetic = true;
            }

            if (is_pointer_arithmetic) {
                // すでに生成した文字列を再利用
                std::string right_expr = generate_expr(*expr.right);
                std::string result = "((void*)((char*)";
                result += left_expr_str;
                result += " " + expr.op + " ";
                result += right_expr;
                result += "))";
                return result;
            }
        } else {
            // 型情報から判定した場合は新たに生成
            std::string left_expr = generate_expr(*expr.left);
            std::string right_expr = generate_expr(*expr.right);

            std::string result = "((void*)((char*)";
            result += left_expr;
            result += " " + expr.op + " ";
            result += right_expr;
            result += "))";
            return result;
        }
    }

    std::string result = "(";
    result += generate_expr(*expr.left);
    result += " " + expr.op + " ";
    result += generate_expr(*expr.right);
    result += ")";
    return result;
}

std::string HIRToCpp::generate_unary_op(const HIRExpr &expr) {
    std::string result = "(";

    // Handle special operators that need conversion
    if (expr.op == "ADDRESS_OF") {
        result += "&";
    } else if (expr.op == "DEREFERENCE") {
        result += "*";
    } else {
        result += expr.op;
    }

    result += generate_expr(*expr.operand);
    result += ")";
    return result;
}

std::string HIRToCpp::generate_function_call(const HIRExpr &expr) {
    // call_function_pointerの特別処理
    if (expr.func_name == "call_function_pointer" &&
        expr.arguments.size() >= 1) {
        // 関数ポインタ呼び出し: ((RetType(*)(Args...))fn_ptr)(args...)
        // ジェネリックコンテキストでは T を使用
        std::string fn_ptr = generate_expr(expr.arguments[0]);
        std::string result = "((int(*)(";

        // ジェネリック型パラメータがあれば使用
        if (!current_generic_params.empty()) {
            for (size_t i = 1; i < expr.arguments.size(); i++) {
                if (i > 1)
                    result += ", ";
                result += current_generic_params[0]; // 全引数を T として扱う
            }
        } else {
            // パラメータ型が不明な場合はvoidに
            for (size_t i = 1; i < expr.arguments.size(); i++) {
                if (i > 1)
                    result += ", ";
                result += "int"; // デフォルトはint
            }
        }

        result += "))" + fn_ptr + ")(";

        // 引数を追加（最初の引数は関数ポインタなのでスキップ）
        for (size_t i = 1; i < expr.arguments.size(); i++) {
            if (i > 1)
                result += ", ";
            result += generate_expr(expr.arguments[i]);
        }

        result += ")";
        return result;
    }

    // FFI関数呼び出しの処理（module.function形式）
    std::string func_name = expr.func_name;
    if (func_name.find('.') != std::string::npos) {
        // module.function -> module_function (ラッパー関数を使用)
        size_t dot_pos = func_name.find('.');
        std::string module = func_name.substr(0, dot_pos);
        std::string function = func_name.substr(dot_pos + 1);
        func_name = module + "_" + function;
    } else {
        // 組み込み関数はプレフィックスを付けない
        if (func_name != "println" && func_name != "print" && func_name != "hex") {
            func_name = add_hir_prefix(func_name);
        }
    }

    std::string result = func_name;

    // CB_HIR_array_getの場合、ジェネリック型パラメータを明示的に指定
    if (expr.func_name == "array_get" && !current_generic_params.empty()) {
        result += "<" + current_generic_params[0] + ">";
    }

    result += "(";

    for (size_t i = 0; i < expr.arguments.size(); i++) {
        if (i > 0)
            result += ", ";
        result += generate_expr(expr.arguments[i]);
    }

    result += ")";
    return result;
}

std::string HIRToCpp::generate_method_call(const HIRExpr &expr) {
    // FFI module call check: module.function(args)
    // If receiver is a simple variable (module name), treat as FFI call
    // IMPORTANT: Only treat as FFI if receiver is explicitly a known FFI module
    // For now, we disable this heuristic and rely on explicit FFI syntax
    /*
    if (expr.receiver && expr.receiver->kind == HIRExpr::ExprKind::Variable) {
        // Check if this looks like an FFI call (receiver is module name)
        // FFI modules are typically short lowercase names like 'm', 'c', etc.
        std::string receiver_name = expr.receiver->var_name;

        // If the receiver is not prefixed and looks like a module name,
        // treat as FFI call: module.function -> module_function
        // Skip if receiver name looks like a regular variable (longer names, or has CB_HIR_ prefix)
        if (receiver_name.find("CB_HIR_") == std::string::npos &&
            receiver_name.length() <= 2 &&  // Changed from 3 to 2 - only single letter modules
            std::islower(receiver_name[0])) { // Must start with lowercase
            std::string result = receiver_name + "_" + expr.method_name + "(";
            for (size_t i = 0; i < expr.arguments.size(); i++) {
                if (i > 0)
                    result += ", ";
                result += generate_expr(expr.arguments[i]);
            }
            result += ")";
            return result;
        }
    }
    */

    // Determine if we should use -> or .
    bool use_arrow = expr.is_arrow ||
                     (expr.receiver &&
                      expr.receiver->type.kind == HIRType::TypeKind::Pointer);

    // Also check if the receiver is a variable in the current function's
    // parameters
    if (!use_arrow && expr.receiver &&
        expr.receiver->kind == HIRExpr::ExprKind::Variable) {
        if (!current_function_params.empty()) {
            auto it = current_function_params.find(expr.receiver->var_name);
            if (it != current_function_params.end() &&
                it->second.kind == HIRType::TypeKind::Pointer) {
                use_arrow = true;
            }
        }
    }

    // Generate the receiver expression
    std::string result = generate_expr(*expr.receiver);
    result += (use_arrow ? "->" : ".") + expr.method_name + "(";

    for (size_t i = 0; i < expr.arguments.size(); i++) {
        if (i > 0)
            result += ", ";
        result += generate_expr(expr.arguments[i]);
    }

    result += ")";
    return result;
}

std::string HIRToCpp::generate_member_access(const HIRExpr &expr) {
    std::string object_str = generate_expr(*expr.object);
    std::string result = object_str;
    result += expr.is_arrow ? "->" : ".";
    result += expr.member_name;
    
    if (expr.is_arrow) {
        debug_msg(DebugMsgId::CODEGEN_CPP_POINTER_ARROW, 
                  object_str.c_str(), expr.member_name.c_str());
    } else {
        debug_msg(DebugMsgId::CODEGEN_CPP_EXPR_MEMBER_ACCESS, expr.member_name.c_str());
    }
    
    return result;
}

std::string HIRToCpp::generate_array_access(const HIRExpr &expr) {
    std::string result = generate_expr(*expr.array);
    result += "[";
    result += generate_expr(*expr.index);
    result += "]";
    return result;
}

std::string HIRToCpp::generate_cast(const HIRExpr &expr) {
    // キャスト式が空または無効な場合、C形式のキャストにフォールバック
    if (!expr.cast_expr) {
        // キャスト対象がない場合は型のデフォルト値を生成
        // This should not happen in valid code
        std::string type_str = generate_type(expr.cast_type);
        return "/* CAST ERROR: no cast_expr */ " + type_str + "{}";
    }

    std::string cast_value = generate_expr(*expr.cast_expr);

    // 生成された式が空の場合、エラーメッセージを含める
    if (cast_value.empty() || cast_value == "/* unsupported expr */") {
        std::string type_str = generate_type(expr.cast_type);
        // Instead of returning empty {}, use C-style cast with a warning
        return "/* CAST ERROR: empty cast_value */ (" + type_str + ")nullptr";
    }

    // 通常のキャスト - C-style castを使用（互換性のため）
    std::string result = "(";
    result += generate_type(expr.cast_type);
    result += ")";
    result += cast_value;
    return result;
}

std::string HIRToCpp::generate_ternary(const HIRExpr &expr) {
    std::string result = "(";
    result += generate_expr(*expr.condition);
    result += " ? ";
    result += generate_expr(*expr.then_expr);
    result += " : ";
    result += generate_expr(*expr.else_expr);
    result += ")";
    return result;
}

std::string HIRToCpp::generate_lambda(const HIRExpr &expr) {
    std::string result = "[](";

    // パラメータ
    for (size_t i = 0; i < expr.lambda_params.size(); i++) {
        if (i > 0)
            result += ", ";
        const auto &param = expr.lambda_params[i];
        if (param.is_const)
            result += "const ";
        result += generate_type(param.type);
        result += " " + add_hir_prefix(param.name);
    }

    result += ") -> ";
    result += generate_type(expr.lambda_return_type);
    result += " { ";

    // Generate lambda body if it exists
    if (expr.lambda_body) {
        // For now, generate a simplified body
        // TODO: Properly generate complex lambda bodies
        if (expr.lambda_body->kind == HIRStmt::StmtKind::Return &&
            expr.lambda_body->return_expr) {
            result += "return ";
            result += generate_expr(*expr.lambda_body->return_expr);
            result += "; ";
        } else if (expr.lambda_body->kind == HIRStmt::StmtKind::Block) {
            // Handle block statements
            for (const auto& stmt : expr.lambda_body->block_stmts) {
                if (stmt.kind == HIRStmt::StmtKind::Return && stmt.return_expr) {
                    result += "return ";
                    result += generate_expr(*stmt.return_expr);
                    result += "; ";
                    break;  // Only handle first return for now
                }
            }
        } else {
            result += "/* complex lambda body */ ";
        }
    } else {
        result += "/* empty lambda */ ";
    }

    result += "}";

    return result;
}

std::string HIRToCpp::generate_struct_literal(const HIRExpr &expr) {
    std::string result = expr.struct_type_name + "{";

    // Check if we have named fields (designated initializers)
    bool use_named = !expr.field_names.empty() &&
                     expr.field_names.size() == expr.field_values.size();

    for (size_t i = 0; i < expr.field_values.size(); i++) {
        if (i > 0)
            result += ", ";

        if (use_named && !expr.field_names[i].empty()) {
            // C++20 designated initializer: .field = value
            result += "." + expr.field_names[i] + " = ";
        }
        result += generate_expr(expr.field_values[i]);
    }

    result += "}";
    return result;
}

std::string HIRToCpp::generate_array_literal(const HIRExpr &expr) {
    // 多次元配列かどうかチェック（最初の要素がArrayLiteralかどうか）
    bool is_multidim = false;
    if (!expr.array_elements.empty() && 
        expr.array_elements[0].kind == HIRExpr::ExprKind::ArrayLiteral) {
        is_multidim = true;
    }
    
    std::string result = "{";
    
    // 多次元配列の場合、外側の{}を追加
    if (is_multidim) {
        result = "{{";
    }

    for (size_t i = 0; i < expr.array_elements.size(); i++) {
        if (i > 0)
            result += ", ";
        result += generate_expr(expr.array_elements[i]);
    }

    result += "}";
    
    // 多次元配列の場合、外側の}を追加
    if (is_multidim) {
        result += "}";
    }
    
    return result;
}

std::string HIRToCpp::generate_address_of(const HIRExpr &expr) {
    std::string operand_str = generate_expr(*expr.operand);
    debug_msg(DebugMsgId::CODEGEN_CPP_POINTER_ADDRESS_OF, operand_str.c_str());
    return "&(" + operand_str + ")";
}

std::string HIRToCpp::generate_dereference(const HIRExpr &expr) {
    if (debug_mode) {
        std::cerr << "[CODEGEN_DEREF] Dereference: has_operand=" 
                  << (expr.operand != nullptr) << std::endl;
    }
    
    if (!expr.operand) {
        std::cerr << "[ERROR] Dereference expression has null operand!" << std::endl;
        return "*(nullptr /* ERROR */)";
    }
    
    std::string operand_str = generate_expr(*expr.operand);
    debug_msg(DebugMsgId::CODEGEN_CPP_POINTER_DEREF, operand_str.c_str());
    
    if (debug_mode) {
        std::cerr << "[CODEGEN_DEREF] Generated: *(" << operand_str << ")" << std::endl;
    }
    
    return "*(" + operand_str + ")";
}

std::string HIRToCpp::generate_sizeof(const HIRExpr &expr) {
    if (expr.sizeof_expr) {
        std::string expr_str = generate_expr(*expr.sizeof_expr);
        debug_msg(DebugMsgId::CODEGEN_CPP_EXPR_START, "sizeof(expr)");
        return "sizeof(" + expr_str + ")";
    } else {
        std::string type_str = generate_type(expr.sizeof_type);
        debug_msg(DebugMsgId::CODEGEN_CPP_EXPR_START, "sizeof(type)");
        return "sizeof(" + type_str + ")";
    }
}

std::string HIRToCpp::generate_new(const HIRExpr &expr) {
    if (debug_mode) {
        std::cerr << "[CODEGEN_NEW] Starting: kind=" << static_cast<int>(expr.new_type.kind)
                  << ", name=" << expr.new_type.name 
                  << ", has_inner=" << (expr.new_type.inner_type != nullptr) << std::endl;
    }
    
    std::string result = "new ";
    
    // 配列の場合は要素型だけ取得
    if (expr.new_type.kind == HIRType::TypeKind::Array && expr.new_type.inner_type) {
        std::string element_type = generate_type(*expr.new_type.inner_type);
        result += element_type;
        
        if (debug_mode) {
            std::cerr << "[CODEGEN_NEW] Array element type: " << element_type << std::endl;
        }
        
        // 配列サイズを追加
        if (expr.new_type.array_size > 0) {
            result += "[" + std::to_string(expr.new_type.array_size) + "]";
        } else if (!expr.new_type.array_dimensions.empty() && expr.new_type.array_dimensions[0] > 0) {
            result += "[" + std::to_string(expr.new_type.array_dimensions[0]) + "]";
        }
    } else {
        std::string type_str = generate_type(expr.new_type);
        debug_msg(DebugMsgId::CODEGEN_CPP_EXPR_NEW, type_str.c_str());
        
        if (debug_mode) {
            std::cerr << "[CODEGEN_NEW] Type generated: " << type_str << std::endl;
        }
        
        result += type_str;

        // コンストラクタ引数がある場合
        if (!expr.new_args.empty()) {
            result += "(";
            for (size_t i = 0; i < expr.new_args.size(); i++) {
                if (i > 0)
                    result += ", ";
                result += generate_expr(expr.new_args[i]);
            }
            result += ")";
        }
        // デフォルト初期化（値初期化）
        else {
            result += "()";  // zero-initialization for primitives
        }
    }

    return result;
}

std::string HIRToCpp::generate_await(const HIRExpr &expr) {
    // Cb's await just accesses the value field of the Future
    return "(" + generate_expr(*expr.operand) + ").value";
}

std::string HIRToCpp::generate_pre_incdec(const HIRExpr &expr) {
    return expr.op + generate_expr(*expr.operand);
}

std::string HIRToCpp::generate_post_incdec(const HIRExpr &expr) {
    return generate_expr(*expr.operand) + expr.op;
}

} // namespace codegen
} // namespace cb
