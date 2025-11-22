/**
 * @file hir_expr_converter.cpp
 * @brief HIR Expression Converter - AST Expression to HIR
 */

#include "hir_expr_converter.h"
#include "../../../common/debug.h"
#include "hir_builder.h"
#include "hir_generator.h"
#include <iostream>

namespace cb {
namespace ir {

using namespace hir;

HIRExprConverter::HIRExprConverter(HIRGenerator *generator)
    : generator_(generator) {}

HIRExprConverter::~HIRExprConverter() {}

HIRExpr HIRExprConverter::convert_expr(const ASTNode *node) {
    HIRExpr expr;

    if (!node) {
        expr.kind = HIRExpr::ExprKind::Literal;
        return expr;
    }

    expr.location = generator_->convert_location(node->location);
    expr.type = generator_->convert_type(node->type_info, node->type_name);

    switch (node->node_type) {
    case ASTNodeType::AST_NUMBER: {
        expr.kind = HIRExpr::ExprKind::Literal;

        // 浮動小数点リテラルの場合はdouble_valueを使用
        if (node->is_float_literal || node->type_info == TYPE_FLOAT ||
            node->type_info == TYPE_DOUBLE) {
            expr.literal_value = std::to_string(node->double_value);
        } else {
            expr.literal_value = std::to_string(node->int_value);
        }

        expr.literal_type = generator_->convert_type(node->type_info);
        break;
    }

    case ASTNodeType::AST_STRING_LITERAL: {
        expr.kind = HIRExpr::ExprKind::Literal;
        expr.literal_value = node->str_value;
        expr.literal_type = generator_->convert_type(TYPE_STRING);
        break;
    }

    // v0.14.0: String interpolation support
    case ASTNodeType::AST_INTERPOLATED_STRING: {
        // 補間文字列は複数のセグメントを連結したBinaryOp(+)として変換
        if (node->interpolation_segments.empty()) {
            expr.kind = HIRExpr::ExprKind::Literal;
            expr.literal_value = "";
            expr.literal_type = generator_->convert_type(TYPE_STRING);
            break;
        }

        if (node->interpolation_segments.size() == 1) {
            // セグメントが1つだけの場合、そのまま返す
            return generator_->convert_expr(
                node->interpolation_segments[0].get());
        }

        // 複数セグメントの場合、順次連結
        auto result =
            generator_->convert_expr(node->interpolation_segments[0].get());
        for (size_t i = 1; i < node->interpolation_segments.size(); i++) {
            HIRExpr concat;
            concat.kind = HIRExpr::ExprKind::BinaryOp;
            concat.op = "+";
            concat.left = std::make_unique<HIRExpr>(std::move(result));
            concat.right = std::make_unique<HIRExpr>(generator_->convert_expr(
                node->interpolation_segments[i].get()));
            result = std::move(concat);
        }
        return result;
    }

    case ASTNodeType::AST_STRING_INTERPOLATION_SEGMENT: {
        // セグメントは文字列リテラルまたは式
        if (node->is_interpolation_expr && node->left) {
            // 式セグメント - HIRに変換して型チェック
            auto inner_expr = generator_->convert_expr(node->left.get());

            // 内部式の型をチェック
            bool is_string_type = false;
            if (inner_expr.kind == HIRExpr::ExprKind::Variable) {
                // 変数の型を推定（これは不完全なので、安全側に倒す）
                is_string_type = false; // デフォルトではto_stringを使う
            } else if (inner_expr.kind == HIRExpr::ExprKind::Literal) {
                is_string_type =
                    (inner_expr.literal_type.kind == HIRType::TypeKind::String);
            }

            if (is_string_type) {
                // 既に文字列型の場合はそのまま返す
                return inner_expr;
            } else {
                // 数値型などの場合はto_stringでラップするが、
                // C++では+演算子がstringとの結合を自動処理するので
                // 実際にはstd::to_stringヘルパー関数を使う
                expr.kind = HIRExpr::ExprKind::FunctionCall;
                expr.func_name = "CB_HIR_to_string_helper";
                expr.arguments.push_back(std::move(inner_expr));
            }
        } else if (node->is_interpolation_text) {
            // 文字列リテラルセグメント
            expr.kind = HIRExpr::ExprKind::Literal;
            expr.literal_value = node->str_value;
            expr.literal_type = generator_->convert_type(TYPE_STRING);
        } else {
            // Fallback: use str_value
            expr.kind = HIRExpr::ExprKind::Literal;
            expr.literal_value = node->str_value;
            expr.literal_type = generator_->convert_type(TYPE_STRING);
        }
        break;
    }

    case ASTNodeType::AST_VARIABLE:
    case ASTNodeType::AST_IDENTIFIER: {
        expr.kind = HIRExpr::ExprKind::Variable;
        expr.var_name = node->name;

        // Lookup variable type from symbol table for type inference
        auto it = generator_->variable_types_.find(node->name);
        if (it != generator_->variable_types_.end()) {
            expr.type = it->second;
        }
        break;
    }

    // v0.14.0: enum値アクセス (EnumName::member)
    case ASTNodeType::AST_ENUM_ACCESS: {
        expr.kind = HIRExpr::ExprKind::Variable;
        // EnumName::member 形式の変数名として扱う
        expr.var_name = node->enum_name + "::" + node->enum_member;
        break;
    }

    case ASTNodeType::AST_BINARY_OP: {
        expr.kind = HIRExpr::ExprKind::BinaryOp;
        expr.op = node->op;
        expr.left = std::make_unique<HIRExpr>(
            generator_->convert_expr(node->left.get()));
        expr.right = std::make_unique<HIRExpr>(
            generator_->convert_expr(node->right.get()));
        break;
    }

    case ASTNodeType::AST_UNARY_OP: {
        // Special case: await is treated as Await expr kind
        if (node->op == "await") {
            expr.kind = HIRExpr::ExprKind::Await;
            expr.operand = std::make_unique<HIRExpr>(
                generator_->convert_expr(node->left.get()));
        } else if (node->op == "&") {
            // Address-of operator: &expr
            expr.kind = HIRExpr::ExprKind::AddressOf;
            expr.operand = std::make_unique<HIRExpr>(
                generator_->convert_expr(node->left.get()));
            // Type should be pointer to operand's type
            // Type inference is done by the type checker or code generator
        } else if (node->op == "*") {
            // Dereference operator: *expr
            expr.kind = HIRExpr::ExprKind::Dereference;
            expr.operand = std::make_unique<HIRExpr>(
                generator_->convert_expr(node->left.get()));
            // Type should be the pointee type
            // Type inference is done by the type checker or code generator
        } else {
            expr.kind = HIRExpr::ExprKind::UnaryOp;
            expr.op = node->op;
            expr.operand = std::make_unique<HIRExpr>(
                generator_->convert_expr(node->left.get()));
        }
        break;
    }

    case ASTNodeType::AST_FUNC_CALL: {
        // v0.14.0: メソッド呼び出しのサポート (obj.method(), ptr->method())
        if (node->left) {
            // レシーバーオブジェクトがある場合はメソッド呼び出し
            expr.kind = HIRExpr::ExprKind::MethodCall;
            expr.receiver = std::make_unique<HIRExpr>(
                generator_->convert_expr(node->left.get()));
            expr.method_name = node->name;
            expr.is_arrow = node->is_arrow_call;

            for (const auto &arg : node->arguments) {
                expr.arguments.push_back(generator_->convert_expr(arg.get()));
            }
        } else {
            // 通常の関数呼び出し
            expr.kind = HIRExpr::ExprKind::FunctionCall;

            // v0.14.0: 修飾名のサポート (m.sqrt, c.abs)
            if (node->is_qualified_call && !node->qualified_name.empty()) {
                expr.func_name = node->qualified_name;
            } else {
                expr.func_name = node->name;
            }

            for (const auto &arg : node->arguments) {
                expr.arguments.push_back(generator_->convert_expr(arg.get()));
            }
        }
        break;
    }

    case ASTNodeType::AST_MEMBER_ACCESS: {
        expr.kind = HIRExpr::ExprKind::MemberAccess;
        expr.object = std::make_unique<HIRExpr>(
            generator_->convert_expr(node->left.get()));
        expr.member_name = node->name;
        break;
    }

    case ASTNodeType::AST_ARROW_ACCESS: {
        expr.kind = HIRExpr::ExprKind::MemberAccess;
        expr.object = std::make_unique<HIRExpr>(
            generator_->convert_expr(node->left.get()));
        expr.member_name = node->name;
        expr.is_arrow = true;
        break;
    }

    case ASTNodeType::AST_ARRAY_REF: {
        expr.kind = HIRExpr::ExprKind::ArrayAccess;
        expr.array = std::make_unique<HIRExpr>(
            generator_->convert_expr(node->left.get()));
        expr.index = std::make_unique<HIRExpr>(
            generator_->convert_expr(node->array_index.get()));
        break;
    }

    case ASTNodeType::AST_CAST_EXPR: {
        expr.kind = HIRExpr::ExprKind::Cast;
        // Cast target is in cast_expr field, not left
        if (node->cast_expr) {
            expr.cast_expr = std::make_unique<HIRExpr>(
                generator_->convert_expr(node->cast_expr.get()));
        } else if (debug_mode) {
            fprintf(stderr, "[HIR_CAST] Warning: Cast expression has no "
                            "cast_expr (target)\n");
        }
        // Use cast_type_info and cast_target_type if available
        if (node->cast_type_info != TYPE_UNKNOWN) {
            expr.cast_type = generator_->convert_type(node->cast_type_info,
                                                      node->cast_target_type);
        } else {
            expr.cast_type =
                generator_->convert_type(node->type_info, node->type_name);
        }
        break;
    }

    case ASTNodeType::AST_TERNARY_OP: {
        expr.kind = HIRExpr::ExprKind::Ternary;
        // パーサーは left=condition, right=true_expr, third=false_expr
        // として格納
        expr.condition = std::make_unique<HIRExpr>(
            generator_->convert_expr(node->left.get()));
        expr.then_expr = std::make_unique<HIRExpr>(
            generator_->convert_expr(node->right.get()));
        expr.else_expr = std::make_unique<HIRExpr>(
            generator_->convert_expr(node->third.get()));
        break;
    }

    // v0.14.0: 範囲式 (start...end)
    case ASTNodeType::AST_RANGE_EXPR: {
        expr.kind = HIRExpr::ExprKind::Range;
        if (node->range_start) {
            expr.range_start = std::make_unique<HIRExpr>(
                generator_->convert_expr(node->range_start.get()));
        }
        if (node->range_end) {
            expr.range_end = std::make_unique<HIRExpr>(
                generator_->convert_expr(node->range_end.get()));
        }
        break;
    }

    case ASTNodeType::AST_STRUCT_LITERAL: {
        expr.kind = HIRExpr::ExprKind::StructLiteral;
        expr.struct_type_name = node->type_name;

        // 名前付き初期化: {name: value, ...}
        // パーサーはargumentsにAST_ASSIGNノードを保存する
        bool has_named_init = false;
        for (const auto &arg : node->arguments) {
            if (arg->node_type == ASTNodeType::AST_ASSIGN) {
                has_named_init = true;
                expr.field_names.push_back(arg->name);
                if (arg->right) {
                    expr.field_values.push_back(
                        generator_->convert_expr(arg->right.get()));
                }
            }
        }

        // 位置ベース初期化: {value1, value2, ...}
        // Only process as positional if no named initialization was found
        if (!has_named_init) {
            for (const auto &arg : node->arguments) {
                expr.field_values.push_back(
                    generator_->convert_expr(arg.get()));
            }
        }
        break;
    }

    case ASTNodeType::AST_ARRAY_LITERAL: {
        expr.kind = HIRExpr::ExprKind::ArrayLiteral;
        // Array literal elements are stored in the arguments field, not
        // children
        for (const auto &element : node->arguments) {
            expr.array_elements.push_back(
                generator_->convert_expr(element.get()));
        }
        break;
    }

    case ASTNodeType::AST_NULLPTR: {
        expr.kind = HIRExpr::ExprKind::Literal;
        expr.literal_value = "nullptr";
        expr.literal_type = generator_->convert_type(TYPE_NULLPTR);
        break;
    }

        // v0.14.0: 追加のHIR式サポート
        // TODO:
        // これらのASTノードタイプは将来実装予定、または既存のAST_UNARY_OPで処理
        // case ASTNodeType::AST_ADDRESS_OF: {
        //     expr.kind = HIRExpr::ExprKind::AddressOf;
        //     expr.operand =
        //     std::make_unique<HIRExpr>(generator_->convert_expr(node->left.get()));
        //     break;
        // }

        // case ASTNodeType::AST_DEREFERENCE: {
        //     expr.kind = HIRExpr::ExprKind::Dereference;
        //     expr.operand =
        //     std::make_unique<HIRExpr>(generator_->convert_expr(node->left.get()));
        //     break;
        // }

    case ASTNodeType::AST_SIZEOF_EXPR: {
        expr.kind = HIRExpr::ExprKind::SizeOf;
        if (node->left) {
            expr.sizeof_expr = std::make_unique<HIRExpr>(
                generator_->convert_expr(node->left.get()));
        } else if (node->sizeof_expr) {
            expr.sizeof_expr = std::make_unique<HIRExpr>(
                generator_->convert_expr(node->sizeof_expr.get()));
        } else {
            // Use sizeof_type_name if available, fallback to type_name
            std::string type_name_to_use = !node->sizeof_type_name.empty()
                                               ? node->sizeof_type_name
                                               : node->type_name;

            // Check if this is a generic type parameter (single uppercase
            // letter or known generic)
            bool is_generic = false;
            if (type_name_to_use.length() == 1 &&
                std::isupper(type_name_to_use[0])) {
                is_generic = true;
            } else if (type_name_to_use == "T" || type_name_to_use == "K" ||
                       type_name_to_use == "V" || type_name_to_use == "U" ||
                       type_name_to_use == "E") {
                is_generic = true;
            }

            // For sizeof, we want to preserve the exact type name from the
            // source So we create a HIRType with the appropriate kind but keep
            // the original name
            if (is_generic) {
                expr.sizeof_type =
                    generator_->convert_type(TYPE_GENERIC, type_name_to_use);
            } else {
                // Determine type kind from type name for sizeof
                TypeInfo inferred_type = TYPE_UNKNOWN;
                if (type_name_to_use.find('*') != std::string::npos) {
                    inferred_type = TYPE_POINTER;
                } else if (type_name_to_use == "int") {
                    inferred_type = TYPE_INT;
                } else if (type_name_to_use == "void") {
                    inferred_type = TYPE_VOID;
                } else if (type_name_to_use == "long") {
                    inferred_type = TYPE_LONG;
                } else if (type_name_to_use == "char") {
                    inferred_type = TYPE_CHAR;
                } else if (type_name_to_use == "float") {
                    inferred_type = TYPE_FLOAT;
                } else if (type_name_to_use == "double") {
                    inferred_type = TYPE_DOUBLE;
                } else if (type_name_to_use == "bool") {
                    inferred_type = TYPE_BOOL;
                } else {
                    // Assume it's a struct or unknown type, preserve the name
                    inferred_type = TYPE_STRUCT;
                }
                expr.sizeof_type =
                    generator_->convert_type(inferred_type, type_name_to_use);
            }
        }
        break;
    }

    case ASTNodeType::AST_PRE_INCDEC: {
        expr.kind = HIRExpr::ExprKind::PreIncDec;
        expr.op = node->op;
        expr.operand = std::make_unique<HIRExpr>(
            generator_->convert_expr(node->left.get()));
        break;
    }

    case ASTNodeType::AST_POST_INCDEC: {
        expr.kind = HIRExpr::ExprKind::PostIncDec;
        expr.op = node->op;
        expr.operand = std::make_unique<HIRExpr>(
            generator_->convert_expr(node->left.get()));
        break;
    }

    case ASTNodeType::AST_NEW_EXPR: {
        expr.kind = HIRExpr::ExprKind::New;
        // new式は new_type_nameを使用
        std::string type_name =
            node->new_type_name.empty() ? node->type_name : node->new_type_name;

        if (debug_mode) {
            std::cerr << "[HIR_EXPR] New expression: type_name=" << type_name
                      << ", type_info=" << node->type_info << std::endl;
        }

        // 型情報を推測（型名を優先）
        TypeInfo type_info = TYPE_STRUCT; // デフォルトは構造体

        if (type_name == "int")
            type_info = TYPE_INT;
        else if (type_name == "long")
            type_info = TYPE_LONG;
        else if (type_name == "short")
            type_info = TYPE_SHORT;
        else if (type_name == "tiny")
            type_info = TYPE_TINY;
        else if (type_name == "char")
            type_info = TYPE_CHAR;
        else if (type_name == "bool")
            type_info = TYPE_BOOL;
        else if (type_name == "float")
            type_info = TYPE_FLOAT;
        else if (type_name == "double")
            type_info = TYPE_DOUBLE;
        else if (type_name == "string")
            type_info = TYPE_STRING;
        else if (type_name == "void")
            type_info = TYPE_VOID;

        if (debug_mode) {
            std::cerr << "[HIR_EXPR] New resolved type_info=" << type_info
                      << std::endl;
        }

        // 配列の場合の処理
        if (node->is_array_new && node->new_array_size) {
            // 配列サイズを取得（リテラルの場合）
            int array_size = -1;
            if (node->new_array_size->node_type == ASTNodeType::AST_NUMBER) {
                array_size = node->new_array_size->int_value;
            }

            // 配列型を作成
            expr.new_type.kind = HIRType::TypeKind::Array;
            expr.new_type.inner_type = std::make_unique<HIRType>();

            // 要素型を設定
            *expr.new_type.inner_type =
                generator_->convert_type(type_info, type_name);
            expr.new_type.array_size = array_size;
            if (!expr.new_type.array_dimensions.empty()) {
                expr.new_type.array_dimensions.clear();
            }
            expr.new_type.array_dimensions.push_back(array_size);

            if (debug_mode) {
                std::cerr << "[HIR_EXPR] Array new: element_type=" << type_name
                          << ", size=" << array_size << std::endl;
            }
        } else {
            // 通常の型
            if (debug_mode) {
                std::cerr << "[HIR_EXPR] Converting type for new: type_info="
                          << type_info << ", type_name=" << type_name
                          << std::endl;
            }

            expr.new_type = generator_->convert_type(type_info, type_name);
        }

        if (debug_mode) {
            std::cerr << "[HIR_EXPR] New type converted successfully"
                      << std::endl;
        }

        // コンストラクタ引数
        for (const auto &arg : node->arguments) {
            expr.new_args.push_back(generator_->convert_expr(arg.get()));
        }
        break;
    }

    case ASTNodeType::AST_LAMBDA_EXPR: {
        expr.kind = HIRExpr::ExprKind::Lambda;
        // ラムダパラメータの変換
        for (const auto &param : node->parameters) {
            HIRExpr::LambdaParameter hir_param;
            hir_param.name = param->name;
            hir_param.type =
                generator_->convert_type(param->type_info, param->type_name);
            hir_param.is_const = param->is_const;
            expr.lambda_params.push_back(hir_param);
        }
        expr.lambda_return_type =
            generator_->convert_type(node->type_info, node->return_type_name);
        // パーサーはlambda_bodyフィールドに本体を設定するので、それを参照する
        if (node->lambda_body) {
            expr.lambda_body = std::make_unique<HIRStmt>(
                generator_->convert_stmt(node->lambda_body.get()));
        }
        break;
    }

        // TODO: メソッド呼び出しは通常の関数呼び出しとして処理されるか、
        // または別のノードタイプで実装される可能性がある
        // case ASTNodeType::AST_METHOD_CALL: {
        //     expr.kind = HIRExpr::ExprKind::MethodCall;
        //     expr.receiver =
        //     std::make_unique<HIRExpr>(generator_->convert_expr(node->left.get()));
        //     expr.method_name = node->name;
        //     for (const auto &arg : node->arguments) {
        //         expr.arguments.push_back(generator_->convert_expr(arg.get()));
        //     }
        //     break;
        // }

    case ASTNodeType::AST_FUNC_PTR_CALL: {
        // 関数ポインタ呼び出し: (*func_ptr)(args...)
        expr.kind = HIRExpr::ExprKind::FunctionCall;
        expr.func_name = "call_function_pointer";

        if (debug_mode) {
            std::cerr << "[HIR_EXPR] Function pointer call" << std::endl;
        }

        // 関数ポインタ式を最初の引数として追加
        if (node->left) {
            expr.arguments.push_back(
                generator_->convert_expr(node->left.get()));
        }

        // 実引数を追加
        for (const auto &arg : node->arguments) {
            expr.arguments.push_back(generator_->convert_expr(arg.get()));
        }

        if (debug_mode) {
            std::cerr << "[HIR_EXPR] Function pointer call with "
                      << node->arguments.size() << " arguments" << std::endl;
        }
        break;
    }

    case ASTNodeType::AST_ENUM_CONSTRUCT: {
        // v0.14.0: Handle enum construction (e.g., Option<int>::Some(42))
        expr.kind = HIRExpr::ExprKind::FunctionCall;

        // Build the full enum constructor name (e.g., "Option<int>::Some")
        std::string full_name = node->enum_name;

        // For built-in types like Option and Result, we need to handle them
        // specially
        if (node->enum_name.find("Option") == 0 ||
            node->enum_name.find("Result") == 0) {
            // Extract the generic type arguments from the enum name
            // e.g., "Option<int>" -> base = "Option", args = "int"
            size_t angle_pos = node->enum_name.find('<');
            if (angle_pos != std::string::npos) {
                std::string base_name = node->enum_name.substr(0, angle_pos);
                std::string type_args = node->enum_name.substr(angle_pos);

                // Build the constructor call (e.g., "Option<int>::Some")
                full_name = base_name + type_args + "::" + node->enum_member;
            } else {
                full_name = node->enum_name + "::" + node->enum_member;
            }
        } else {
            full_name = node->enum_name + "::" + node->enum_member;
        }

        expr.func_name = full_name;

        // Convert the arguments for the enum variant's associated values
        for (const auto &arg : node->arguments) {
            expr.arguments.push_back(generator_->convert_expr(arg.get()));
        }

        if (debug_mode) {
            std::cerr << "[HIR_EXPR] Enum construct: " << full_name << " with "
                      << node->arguments.size() << " arguments" << std::endl;
        }
        break;
    }

    // v0.12.1: エラー伝播演算子 (?)
    case ASTNodeType::AST_ERROR_PROPAGATION: {
        expr.kind = HIRExpr::ExprKind::ErrorPropagation;

        // 内部式を変換（leftフィールドを使用）
        if (node->left) {
            expr.operand = std::make_unique<HIRExpr>(
                generator_->convert_expr(node->left.get()));
        }

        if (debug_mode) {
            std::cerr << "[HIR_EXPR] Error propagation operator (?)"
                      << std::endl;
        }
        break;
    }

    // v0.14.0: Discard variable (_) - 読み込み禁止
    case ASTNodeType::AST_DISCARD_VARIABLE: {
        // Discard変数を式の中で参照することは禁止
        std::string error_msg = "Cannot reference discard variable '_'";
        generator_->report_error(error_msg, node->location);

        // エラー後もコンパイルを続行するため、ダミーのリテラルを返す
        expr.kind = HIRExpr::ExprKind::Literal;
        expr.literal_value = "0";
        expr.literal_type = generator_->convert_type(TYPE_INT);
        break;
    }

    default: {
        std::string error_msg =
            "Unsupported expression type in HIR generation: AST node type " +
            std::to_string(static_cast<int>(node->node_type));
        generator_->report_error(error_msg, node->location);
        expr.kind = HIRExpr::ExprKind::Literal;
        break;
    }
    }

    return expr;
}

} // namespace ir
} // namespace cb
