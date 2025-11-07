#include "special.h"
#include "../../../../common/debug.h"
#include "../../core/pointer_metadata.h"
#include "../../managers/types/enums.h"
#include "../../managers/types/manager.h"
#include "../core/evaluator.h"
#include <functional>
#include <iostream>
#include <stdexcept>

namespace SpecialAccessHelpers {

int64_t evaluate_arrow_access(
    const ASTNode *node, Interpreter &interpreter,
    ExpressionEvaluator &evaluator,
    std::function<int64_t(const ASTNode *)> evaluate_expression_func,
    std::function<Variable(const Variable &, const std::string &)>
        get_struct_member_func) {
    // アロー演算子アクセス: ptr->member は (*ptr).member と等価
    // まず左側のポインタを評価
    debug_msg(DebugMsgId::EXPR_EVAL_START, "Arrow operator member access");

    std::string member_name = node->name;

    // v0.11.0 Week 2 Day 3: ptr[index]->member パターン対応
    // 左側がポインタ配列アクセス (ptr[0])
    // の場合、ReturnExceptionで構造体が返される
    Variable *struct_var = nullptr;
    int64_t ptr_value = 0;

    // v0.11.0 Phase 1a: ジェネリックポインタ変数からの直接アクセス
    // node->left が AST_VARIABLE で、その変数が is_pointer かつ
    // pointer_base_type_name を持つ場合
    if (node->left && node->left->node_type == ASTNodeType::AST_VARIABLE) {
        Variable *ptr_var = interpreter.find_variable(node->left->name);
        if (ptr_var && ptr_var->is_pointer &&
            !ptr_var->pointer_base_type_name.empty()) {

            // ポインタ値を取得
            ptr_value = ptr_var->value;

            // 重要: ジェネリック型でない通常の構造体の場合のみ、
            // ptr_valueをVariable*として扱う。
            // ジェネリック構造体（MapNode<K,V>など）の場合は、
            // 元のコードパス（生メモリアクセス）を使う必要がある。
            bool is_non_generic_struct =
                (ptr_value != 0 && ptr_var->pointer_base_type_name.find('<') ==
                                       std::string::npos);

            if (is_non_generic_struct) {
                struct_var = reinterpret_cast<Variable *>(ptr_value);

                // Variable構造体として有効かチェック
                // typeフィールドがTYPE_STRUCTで、かつis_structフラグが立っているか確認
                bool looks_like_variable =
                    (struct_var->type == TYPE_STRUCT && struct_var->is_struct);

                if (looks_like_variable) {
                    // メンバーを取得
                    Variable member_var =
                        get_struct_member_func(*struct_var, member_name);

                    if (member_var.type == TYPE_STRING) {
                        TypedValue typed_result(
                            static_cast<int64_t>(0),
                            InferredType(TYPE_STRING, "string"));
                        typed_result.string_value = member_var.str_value;
                        typed_result.is_numeric_result = false;
                        evaluator.set_last_typed_result(typed_result);
                        return 0;
                    } else if (member_var.type == TYPE_POINTER) {
                        return member_var.value;
                    } else if (member_var.type == TYPE_FLOAT) {
                        TypedValue typed_result(
                            static_cast<double>(member_var.float_value),
                            InferredType(TYPE_FLOAT, "float"));
                        typed_result.is_numeric_result = true;
                        typed_result.is_float_result = true;
                        evaluator.set_last_typed_result(typed_result);
                        return 0;
                    } else if (member_var.type == TYPE_DOUBLE) {
                        TypedValue typed_result(
                            member_var.double_value,
                            InferredType(TYPE_DOUBLE, "double"));
                        evaluator.set_last_typed_result(typed_result);
                        return 0;
                    } else if (member_var.type == TYPE_QUAD) {
                        TypedValue typed_result(
                            static_cast<long double>(member_var.quad_value),
                            InferredType(TYPE_QUAD, "quad"));
                        evaluator.set_last_typed_result(typed_result);
                        return 0;
                    } else {
                        // 整数型
                        return member_var.value;
                    }
                }
                // looks_like_variableがfalseの場合、以下の通常処理にフォールスルー
            }

            // ジェネリック型を解決
            std::string resolved_type_name =
                interpreter.resolve_type_in_context(
                    ptr_var->pointer_base_type_name);

            // 構造体定義を取得
            const StructDefinition *struct_def =
                interpreter.get_struct_definition(resolved_type_name);

            // 見つからない場合、ベース定義を取得
            if (!struct_def &&
                resolved_type_name.find('<') != std::string::npos) {
                size_t angle_pos = resolved_type_name.find('<');
                std::string base_struct_name =
                    resolved_type_name.substr(0, angle_pos);
                struct_def =
                    interpreter.get_struct_definition(base_struct_name);
            }

            if (!struct_def) {
                throw std::runtime_error(
                    "Cannot find struct definition for pointer type: " +
                    resolved_type_name);
            }

            if (ptr_value == 0) {
                debug_msg(DebugMsgId::GENERIC_DEBUG,
                          "[ARROW_OP] Null pointer access: var='%s', ");
                // nullポインタの場合、デフォルト値を返す
                // メンバーの型を調べて、適切なデフォルト値を設定

                // 構造体定義を取得してメンバーの型を確認
                if (struct_def) {
                    for (const auto &member : struct_def->members) {
                        if (member.name == member_name) {
                            // ポインタメンバーの場合は単純に0を返す
                            if (member.is_pointer) {
                                debug_msg(DebugMsgId::GENERIC_DEBUG,
                                          "[ARROW_OP] Member '%s' is ");
                                // TypedValue
                                // をクリアして、ポインタ値として0を設定
                                TypedValue typed_result(
                                    static_cast<int64_t>(0),
                                    InferredType(TYPE_POINTER, ""));
                                typed_result.is_numeric_result = true;
                                evaluator.set_last_typed_result(typed_result);
                                return 0;
                            }

                            TypeInfo actual_type = member.type;
                            if (actual_type == TYPE_UNKNOWN &&
                                !member.type_alias.empty()) {
                                std::string resolved =
                                    interpreter.resolve_type_in_context(
                                        member.type_alias);
                                if (resolved == "string")
                                    actual_type = TYPE_STRING;
                            }

                            if (actual_type == TYPE_STRING) {
                                debug_msg(DebugMsgId::GENERIC_DEBUG,
                                          "[ARROW_OP] Member '%s' is string, ");
                                TypedValue typed_result(
                                    "", InferredType(TYPE_STRING, "string"));
                                evaluator.set_last_typed_result(typed_result);
                                return 0;
                            }
                            break;
                        }
                    }
                }
                debug_msg(DebugMsgId::GENERIC_DEBUG,
                          "[ARROW_OP] Member '%s' type unknown or numeric, ");
                return 0; // 数値型のデフォルト
            }

            // メンバーを検索してオフセットを計算
            size_t offset = 0;
            bool member_found = false;
            TypeInfo member_type = TYPE_UNKNOWN;
            bool member_is_pointer = false;

            for (const auto &member : struct_def->members) {
                // ジェネリック型パラメータを解決
                TypeInfo actual_type = member.type;
                if (actual_type == TYPE_UNKNOWN && !member.type_alias.empty()) {
                    std::string resolved =
                        interpreter.resolve_type_in_context(member.type_alias);

                    if (resolved == "int")
                        actual_type = TYPE_INT;
                    else if (resolved == "long")
                        actual_type = TYPE_LONG;
                    else if (resolved == "short")
                        actual_type = TYPE_SHORT;
                    else if (resolved == "tiny")
                        actual_type = TYPE_TINY;
                    else if (resolved == "char")
                        actual_type = TYPE_CHAR;
                    else if (resolved == "bool")
                        actual_type = TYPE_BOOL;
                    else if (resolved == "float")
                        actual_type = TYPE_FLOAT;
                    else if (resolved == "double")
                        actual_type = TYPE_DOUBLE;
                    else if (resolved == "string")
                        actual_type = TYPE_STRING;
                    else if (resolved.find('*') != std::string::npos)
                        actual_type = TYPE_POINTER;
                }

                // メンバーのサイズを取得
                size_t member_size = 0;
                if (member.is_pointer || actual_type == TYPE_POINTER) {
                    member_size = sizeof(void *);
                } else {
                    switch (actual_type) {
                    case TYPE_INT:
                        member_size = 4;
                        break;
                    case TYPE_LONG:
                        member_size = 8;
                        break;
                    case TYPE_SHORT:
                        member_size = 2;
                        break;
                    case TYPE_TINY:
                        member_size = 1;
                        break;
                    case TYPE_CHAR:
                        member_size = 1;
                        break;
                    case TYPE_BOOL:
                        member_size = 1;
                        break;
                    case TYPE_FLOAT:
                        member_size = 4;
                        break;
                    case TYPE_DOUBLE:
                        member_size = 8;
                        break;
                    case TYPE_STRING:
                        member_size = sizeof(void *);
                        break;
                    default:
                        member_size = sizeof(void *);
                        break;
                    }
                }

                // アライメントを適用
                size_t alignment = member_size;
                if (alignment > 8)
                    alignment = 8;
                if (alignment > 0) {
                    size_t padding =
                        (alignment - (offset % alignment)) % alignment;
                    offset += padding;
                }

                if (member.name == member_name) {
                    member_found = true;
                    member_type = actual_type;
                    member_is_pointer = member.is_pointer;
                    break;
                }

                offset += member_size;
            }

            if (!member_found) {
                throw std::runtime_error("Member '" + member_name +
                                         "' not found in struct " +
                                         resolved_type_name);
            }

            // メモリから読み取り
            char *base_addr = reinterpret_cast<char *>(ptr_value);
            char *member_addr = base_addr + offset;

            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "[ARROW_OP] Reading from memory: addr=0x%llx, ");

            // 型に応じて読み取り
            if (member_is_pointer || member_type == TYPE_POINTER) {
                int64_t ptr_val = *reinterpret_cast<int64_t *>(member_addr);
                {
                    char dbg_buf[512];
                    snprintf(dbg_buf, sizeof(dbg_buf),
                             "[ARROW_OP] Read pointer value: 0x%llx",
                             (unsigned long long)ptr_val);
                    debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                }
                // TypedValueを設定してポインタ型を明示
                TypedValue typed_result(ptr_val,
                                        InferredType(TYPE_POINTER, "pointer"));
                typed_result.is_pointer = true;
                evaluator.set_last_typed_result(typed_result);
                return ptr_val;
            }

            switch (member_type) {
            case TYPE_INT: {
                int32_t int_val = *reinterpret_cast<int32_t *>(member_addr);
                TypedValue typed_result(static_cast<int64_t>(int_val),
                                        InferredType(TYPE_INT, "int"));
                typed_result.is_numeric_result = true;
                evaluator.set_last_typed_result(typed_result);
                return static_cast<int64_t>(int_val);
            }
            case TYPE_LONG: {
                int64_t long_val = *reinterpret_cast<int64_t *>(member_addr);
                TypedValue typed_result(long_val,
                                        InferredType(TYPE_LONG, "long"));
                typed_result.is_numeric_result = true;
                evaluator.set_last_typed_result(typed_result);
                return long_val;
            }
            case TYPE_SHORT: {
                int16_t short_val = *reinterpret_cast<int16_t *>(member_addr);
                TypedValue typed_result(static_cast<int64_t>(short_val),
                                        InferredType(TYPE_SHORT, "short"));
                typed_result.is_numeric_result = true;
                evaluator.set_last_typed_result(typed_result);
                return static_cast<int64_t>(short_val);
            }
            case TYPE_TINY:
            case TYPE_CHAR: {
                int8_t tiny_val = *reinterpret_cast<int8_t *>(member_addr);
                TypedValue typed_result(
                    static_cast<int64_t>(tiny_val),
                    InferredType(member_type,
                                 member_type == TYPE_TINY ? "tiny" : "char"));
                typed_result.is_numeric_result = true;
                evaluator.set_last_typed_result(typed_result);
                return static_cast<int64_t>(tiny_val);
            }
            case TYPE_BOOL: {
                bool bool_val = *reinterpret_cast<bool *>(member_addr);
                TypedValue typed_result(static_cast<int64_t>(bool_val),
                                        InferredType(TYPE_BOOL, "bool"));
                typed_result.is_numeric_result = true;
                evaluator.set_last_typed_result(typed_result);
                return static_cast<int64_t>(bool_val);
            }
            case TYPE_FLOAT: {
                float float_val = *reinterpret_cast<float *>(member_addr);
                TypedValue typed_result(static_cast<double>(float_val),
                                        InferredType(TYPE_FLOAT, "float"));
                evaluator.set_last_typed_result(typed_result);
                return 0;
            }
            case TYPE_DOUBLE: {
                double double_val = *reinterpret_cast<double *>(member_addr);
                TypedValue typed_result(double_val,
                                        InferredType(TYPE_DOUBLE, "double"));
                evaluator.set_last_typed_result(typed_result);
                return 0;
            }
            case TYPE_STRING: {
                // メモリからC文字列ポインタを読み取り
                const char *str_ptr =
                    *reinterpret_cast<const char **>(member_addr);
                std::string str_val = (str_ptr != nullptr) ? str_ptr : "";
                {
                    char dbg_buf[512];
                    snprintf(dbg_buf, sizeof(dbg_buf),
                             "[ARROW_OP] Read string value: ptr=%p, str='%s'",
                             (void *)str_ptr, str_val.c_str());
                    debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                }
                TypedValue typed_result(str_val,
                                        InferredType(TYPE_STRING, "string"));
                // value
                // フィールドにもポインタを保存（generic型での比較等で使用）
                typed_result.value = reinterpret_cast<int64_t>(str_ptr);
                evaluator.set_last_typed_result(typed_result);
                return 0;
            }
            default:
                throw std::runtime_error(
                    "Unsupported member type for pointer-based read");
            }
        }
    }

    try {
        // ポインタを評価して値を取得
        ptr_value = evaluate_expression_func(node->left.get());
    } catch (const ReturnException &ret) {
        // 構造体が返された場合（ptr[index]からの構造体）
        if (ret.is_struct) {
            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "[ARROW_OP] Caught struct from ptr[index], type='%s'",
                         ret.struct_value.struct_type_name.c_str());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }

            // 構造体からメンバーを取得
            Variable member_var =
                get_struct_member_func(ret.struct_value, member_name);

            if (member_var.type == TYPE_STRING) {
                TypedValue typed_result(static_cast<int64_t>(0),
                                        InferredType(TYPE_STRING, "string"));
                typed_result.string_value = member_var.str_value;
                typed_result.is_numeric_result = false;
                evaluator.set_last_typed_result(typed_result);
                return 0;
            } else if (member_var.type == TYPE_POINTER) {
                return member_var.value;
            } else if (member_var.type == TYPE_FLOAT ||
                       member_var.type == TYPE_DOUBLE ||
                       member_var.type == TYPE_QUAD) {
                return static_cast<int64_t>(member_var.float_value);
            } else {
                return member_var.value;
            }
        } else {
            // その他のReturnExceptionは再投げ
            throw;
        }
    }

    {
        char dbg_buf[512];
        snprintf(dbg_buf, sizeof(dbg_buf),
                 "[ARROW_OP] ptr_value=0x%llx has_meta=%s",
                 static_cast<unsigned long long>(ptr_value),
                 (ptr_value & (1LL << 63)) ? "yes" : "no");
        debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
    }

    if (ptr_value == 0) {
        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "[ARROW_OP] Null pointer access in ReturnException path, ");
        return 0; // nullポインタの場合、デフォルト値を返す
    }

    // メタデータポインタかどうかをチェック（最上位ビットが1）
    bool has_metadata = (ptr_value & (1LL << 63)) != 0;

    if (has_metadata) {
        // メタデータポインタの場合、最上位ビットをクリアして実際のポインタを取得
        int64_t meta_ptr = ptr_value & ~(1LL << 63);
        PointerSystem::PointerMetadata *metadata =
            reinterpret_cast<PointerSystem::PointerMetadata *>(meta_ptr);

        if (!metadata) {
            throw std::runtime_error(
                "Invalid metadata pointer in arrow operator");
        }

        // メタデータの種類に応じて処理
        if (metadata->target_type ==
                PointerSystem::PointerTargetType::VARIABLE &&
            metadata->var_ptr) {
            struct_var = metadata->var_ptr;
        } else if (metadata->target_type ==
                   PointerSystem::PointerTargetType::ARRAY_ELEMENT) {
            // 配列要素の場合、array_valuesから実際の要素ポインタを取得
            // 構造体配列の場合、array_name[index] という変数名を構築
            if (!metadata->array_var) {
                throw std::runtime_error(
                    "Invalid array metadata in arrow operator");
            }

            // 配列要素の変数名を構築: "rectangles[0]" など
            std::string element_name = metadata->array_name + "[" +
                                       std::to_string(metadata->element_index) +
                                       "]";
            struct_var = interpreter.find_variable(element_name);

            if (!struct_var) {
                throw std::runtime_error("Struct array element not found: " +
                                         element_name);
            }
        } else if (!metadata->struct_type_name.empty()) {
            // 構造体ポインタ（型キャスト済み）の場合
            // 生メモリから直接メンバーにアクセス
            const StructDefinition *struct_def =
                interpreter.find_struct_definition(metadata->struct_type_name);

            if (!struct_def) {
                throw std::runtime_error("Struct definition not found: " +
                                         metadata->struct_type_name);
            }

            // メンバーのオフセットを計算（パディング込み）
            size_t offset = 0;
            bool found = false;
            TypeInfo member_type = TYPE_UNKNOWN;
            std::string member_struct_type;

            for (const auto &member : struct_def->members) {
                // ジェネリック型パラメータを解決
                TypeInfo actual_type = member.type;
                if (actual_type == TYPE_UNKNOWN && !member.type_alias.empty()) {
                    std::string resolved =
                        interpreter.resolve_type_in_context(member.type_alias);

                    if (resolved == "int")
                        actual_type = TYPE_INT;
                    else if (resolved == "long")
                        actual_type = TYPE_LONG;
                    else if (resolved == "short")
                        actual_type = TYPE_SHORT;
                    else if (resolved == "tiny")
                        actual_type = TYPE_TINY;
                    else if (resolved == "char")
                        actual_type = TYPE_CHAR;
                    else if (resolved == "bool")
                        actual_type = TYPE_BOOL;
                    else if (resolved == "float")
                        actual_type = TYPE_FLOAT;
                    else if (resolved == "double")
                        actual_type = TYPE_DOUBLE;
                    else if (resolved == "string")
                        actual_type = TYPE_STRING;
                    else if (resolved.find('*') != std::string::npos)
                        actual_type = TYPE_POINTER;
                }

                // メンバーのサイズを計算
                size_t member_size = 0;
                if (member.is_pointer || actual_type == TYPE_POINTER) {
                    member_size = 8;
                } else if (actual_type == TYPE_INT) {
                    member_size = 4;
                } else if (actual_type == TYPE_LONG) {
                    member_size = 8;
                } else if (actual_type == TYPE_SHORT) {
                    member_size = 2;
                } else if (actual_type == TYPE_TINY ||
                           actual_type == TYPE_CHAR) {
                    member_size = 1;
                } else if (actual_type == TYPE_BOOL) {
                    member_size = 1;
                } else if (actual_type == TYPE_FLOAT) {
                    member_size = 4;
                } else if (actual_type == TYPE_DOUBLE) {
                    member_size = 8;
                } else if (actual_type == TYPE_STRING) {
                    member_size = 8;
                } else if (actual_type == TYPE_STRUCT) {
                    // 構造体メンバーのサイズを取得
                    const StructDefinition *member_struct_def =
                        interpreter.find_struct_definition(member.type_alias);
                    if (member_struct_def) {
                        member_size = 0;
                        for (const auto &sub_member :
                             member_struct_def->members) {
                            if (sub_member.type == TYPE_INT) {
                                member_size += 4;
                            } else if (sub_member.type == TYPE_LONG) {
                                member_size += 8;
                            } else if (sub_member.type == TYPE_POINTER) {
                                member_size += 8;
                            }
                        }
                    } else {
                        member_size = 8; // デフォルトサイズ
                    }
                } else {
                    member_size = 4; // デフォルト
                }

                // アライメントを考慮
                size_t alignment = member_size;
                if (alignment > 8)
                    alignment = 8;
                if (alignment > 0) {
                    size_t padding =
                        (alignment - (offset % alignment)) % alignment;
                    offset += padding;
                }

                if (member.name == member_name) {
                    found = true;
                    member_type = member.type;
                    member_struct_type = member.type_alias;
                    break;
                }

                offset += member_size;
            }

            if (!found) {
                throw std::runtime_error("Member '" + member_name +
                                         "' not found in struct " +
                                         metadata->struct_type_name);
            }

            // 生メモリから直接値を読み取り
            void *base_ptr = reinterpret_cast<void *>(metadata->address);
            void *member_ptr = static_cast<char *>(base_ptr) + offset;

            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "[ARROW_OP] Raw memory access: base_ptr=%p offset=%zu ");

            if (member_type == TYPE_INT) {
                int *int_ptr = static_cast<int *>(member_ptr);
                return static_cast<int64_t>(*int_ptr);
            } else if (member_type == TYPE_LONG) {
                int64_t *long_ptr = static_cast<int64_t *>(member_ptr);
                return *long_ptr;
            } else if (member_type == TYPE_POINTER) {
                void **ptr_ptr = static_cast<void **>(member_ptr);
                return reinterpret_cast<int64_t>(*ptr_ptr);
            } else if (member_type == TYPE_STRING) {
                const char **str_ptr = static_cast<const char **>(member_ptr);
                std::string str_val = (*str_ptr != nullptr) ? *str_ptr : "";
                TypedValue typed_result(str_val,
                                        InferredType(TYPE_STRING, "string"));
                evaluator.set_last_typed_result(typed_result);
                return 0;
            } else {
                throw std::runtime_error(
                    "Unsupported member type in raw pointer access");
            }
        } else {
            throw std::runtime_error(
                "Unsupported metadata type in arrow operator");
        }
    } else {
        // メタデータなしのポインタの場合
        std::string struct_type_name;

        // 左側がキャスト式の場合、キャストの型情報を使用
        if (node->left && node->left->node_type == ASTNodeType::AST_CAST_EXPR) {
            if (!node->left->cast_target_type.empty() &&
                node->left->cast_target_type.find('*') != std::string::npos) {
                struct_type_name = node->left->cast_target_type;
                size_t star_pos = struct_type_name.find('*');
                if (star_pos != std::string::npos) {
                    struct_type_name = struct_type_name.substr(0, star_pos);
                }

                debug_msg(
                    DebugMsgId::GENERIC_DEBUG,
                    "[ARROW_OP] Cast expression: cast_target_type='%s', ");
            }
        }
        // 左側が変数参照なら、その変数の型情報をチェック
        else if (node->left &&
                 node->left->node_type == ASTNodeType::AST_VARIABLE) {
            Variable *ptr_var = interpreter.find_variable(node->left->name);

            if (interpreter.is_debug_mode()) {
                debug_msg(
                    DebugMsgId::GENERIC_DEBUG,
                    "[ARROW_OP] Variable '%s' found=%d, type_name='%s', ");
            }

            // v0.11.0 Phase 1a: pointer_base_type_name
            // が設定されている場合はそれを優先
            if (ptr_var && ptr_var->is_pointer &&
                !ptr_var->pointer_base_type_name.empty()) {
                struct_type_name = ptr_var->pointer_base_type_name;

                // ジェネリック型を解決
                struct_type_name =
                    interpreter.resolve_type_in_context(struct_type_name);

                if (interpreter.is_debug_mode()) {
                    debug_msg(DebugMsgId::GENERIC_DEBUG,
                              "[ARROW_OP] Using pointer_base_type_name: '%s' ");
                }
            } else if (ptr_var && !ptr_var->type_name.empty() &&
                       ptr_var->type_name.find('*') != std::string::npos) {
                struct_type_name = ptr_var->type_name;
                size_t star_pos = struct_type_name.find('*');
                if (star_pos != std::string::npos) {
                    struct_type_name = struct_type_name.substr(0, star_pos);
                }
            }
        }

        // 構造体ポインタとして処理
        if (!struct_type_name.empty()) {
            // スペースを除去
            struct_type_name.erase(std::remove_if(struct_type_name.begin(),
                                                  struct_type_name.end(),
                                                  ::isspace),
                                   struct_type_name.end());

            // 構造体定義を取得
            const StructDefinition *struct_def =
                interpreter.find_struct_definition(struct_type_name);

            if (struct_def) {
                // まずVariable*として扱うことを試みる
                // &変数 の場合、ポインタはVariable*を指している
                struct_var = reinterpret_cast<Variable *>(ptr_value);

                // Variable*として有効かチェック：
                // 1. struct_type_nameが一致
                // 2. struct_membersが空でない
                bool is_variable_ptr = false;
                try {
                    if (struct_var && struct_var->type == TYPE_STRUCT &&
                        struct_var->struct_type_name == struct_type_name &&
                        !struct_var->struct_members.empty()) {
                        is_variable_ptr = true;
                        {
                            char dbg_buf[512];
                            snprintf(dbg_buf, sizeof(dbg_buf),
                                     "[ARROW_OP] Treating as Variable* to "
                                     "struct '%s'",
                                     struct_type_name.c_str());
                            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                        }
                    }
                } catch (...) {
                    // ポインタが不正な場合、例外を無視して生メモリアクセスにフォールバック
                    is_variable_ptr = false;
                }

                if (is_variable_ptr || struct_def->has_default_member) {
                    // Variable*として扱う（後続処理で変数テーブルからアクセス）
                    goto variable_access;
                }

                debug_msg(DebugMsgId::GENERIC_DEBUG,
                          "[ARROW_OP] Raw pointer access to struct '%s', ");
                // 生メモリから直接メンバーにアクセス
                void *base_ptr = reinterpret_cast<void *>(ptr_value);

                // メンバーのオフセットを計算
                size_t offset = 0;
                bool found = false;
                TypeInfo member_type = TYPE_UNKNOWN;
                bool member_is_pointer = false;

                for (const auto &member : struct_def->members) {
                    // メンバーのサイズを計算
                    size_t member_size = 0;
                    if (member.is_pointer) {
                        member_size = sizeof(void *);
                    } else {
                        TypeInfo this_member_type = member.type;

                        // ジェネリック型パラメータの解決
                        if (this_member_type == TYPE_UNKNOWN &&
                            !member.type_alias.empty()) {
                            std::string resolved =
                                interpreter.resolve_type_in_context(
                                    member.type_alias);

                            if (resolved == "int")
                                this_member_type = TYPE_INT;
                            else if (resolved == "long")
                                this_member_type = TYPE_LONG;
                            else if (resolved == "short")
                                this_member_type = TYPE_SHORT;
                            else if (resolved == "tiny")
                                this_member_type = TYPE_TINY;
                            else if (resolved == "char")
                                this_member_type = TYPE_CHAR;
                            else if (resolved == "bool")
                                this_member_type = TYPE_BOOL;
                            else if (resolved == "float")
                                this_member_type = TYPE_FLOAT;
                            else if (resolved == "double")
                                this_member_type = TYPE_DOUBLE;
                            else if (resolved == "string")
                                this_member_type = TYPE_STRING;
                            else if (resolved.find('*') != std::string::npos)
                                this_member_type = TYPE_POINTER;
                        }

                        switch (this_member_type) {
                        case TYPE_INT:
                            member_size = 4;
                            break;
                        case TYPE_LONG:
                            member_size = 8;
                            break;
                        case TYPE_SHORT:
                            member_size = 2;
                            break;
                        case TYPE_TINY:
                            member_size = 1;
                            break;
                        case TYPE_CHAR:
                            member_size = 1;
                            break;
                        case TYPE_BOOL:
                            member_size = 1;
                            break;
                        case TYPE_FLOAT:
                            member_size = 4;
                            break;
                        case TYPE_DOUBLE:
                            member_size = 8;
                            break;
                        case TYPE_STRING:
                            member_size = sizeof(void *);
                            break;
                        case TYPE_POINTER:
                            member_size = sizeof(void *);
                            break;
                        default:
                            member_size = sizeof(void *);
                            break;
                        }
                    }

                    // アライメントを考慮
                    size_t alignment = member_size;
                    if (alignment > 8)
                        alignment = 8;
                    if (alignment > 0) {
                        size_t padding =
                            (alignment - (offset % alignment)) % alignment;
                        offset += padding;
                    }

                    // 現在のメンバーが目的のメンバーなら、現在のオフセットを使う
                    if (member.name == member_name) {
                        found = true;
                        member_type = member.type;
                        member_is_pointer = member.is_pointer;

                        // v0.11.0 Phase 1a: ジェネリック型パラメータの解決
                        if (member_type == TYPE_UNKNOWN &&
                            !member.type_alias.empty()) {
                            std::string resolved_member_type =
                                interpreter.resolve_type_in_context(
                                    member.type_alias);

                            if (resolved_member_type == "int")
                                member_type = TYPE_INT;
                            else if (resolved_member_type == "long")
                                member_type = TYPE_LONG;
                            else if (resolved_member_type == "short")
                                member_type = TYPE_SHORT;
                            else if (resolved_member_type == "tiny")
                                member_type = TYPE_TINY;
                            else if (resolved_member_type == "char")
                                member_type = TYPE_CHAR;
                            else if (resolved_member_type == "bool")
                                member_type = TYPE_BOOL;
                            else if (resolved_member_type == "float")
                                member_type = TYPE_FLOAT;
                            else if (resolved_member_type == "double")
                                member_type = TYPE_DOUBLE;
                            else if (resolved_member_type == "string")
                                member_type = TYPE_STRING;
                            else if (resolved_member_type.find('*') !=
                                     std::string::npos)
                                member_type = TYPE_POINTER;

                            if (interpreter.is_debug_mode()) {
                                debug_msg(DebugMsgId::GENERIC_DEBUG,
                                          "[ARROW_OP] Resolved generic ");
                            }
                        }

                        break;
                    }

                    offset += member_size;
                }

                if (!found) {
                    throw std::runtime_error("Member not found: " +
                                             member_name);
                }

                debug_msg(
                    DebugMsgId::GENERIC_DEBUG,
                    "[ARROW_OP] Member '%s' found at offset %zu, type=%d, ");

                // 生メモリから値を読み取り
                void *member_ptr = static_cast<char *>(base_ptr) + offset;

                if (member_is_pointer || member_type == TYPE_POINTER) {
                    void **ptr_ptr = static_cast<void **>(member_ptr);
                    int64_t ptr_val = reinterpret_cast<int64_t>(*ptr_ptr);
                    {
                        char dbg_buf[512];
                        snprintf(
                            dbg_buf, sizeof(dbg_buf),
                            "[ARROW_OP] Read pointer value: 0x%llx from 0x%lx",
                            (unsigned long long)ptr_val,
                            reinterpret_cast<uintptr_t>(member_ptr));
                        debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                    }
                    return ptr_val;
                } else if (member_type == TYPE_INT) {
                    int32_t *int_ptr = static_cast<int32_t *>(member_ptr);
                    int64_t value = static_cast<int64_t>(*int_ptr);
                    {
                        char dbg_buf[512];
                        snprintf(
                            dbg_buf, sizeof(dbg_buf),
                            "[ARROW_OP] Read int32_t value: %lld from 0x%lx",
                            value, reinterpret_cast<uintptr_t>(member_ptr));
                        debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                    }
                    return value;
                } else if (member_type == TYPE_LONG) {
                    int64_t *int_ptr = static_cast<int64_t *>(member_ptr);
                    int64_t value = *int_ptr;
                    {
                        char dbg_buf[512];
                        snprintf(
                            dbg_buf, sizeof(dbg_buf),
                            "[ARROW_OP] Read int64_t value: %lld from 0x%lx",
                            value, reinterpret_cast<uintptr_t>(member_ptr));
                        debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                    }
                    return value;
                } else if (member_type == TYPE_FLOAT) {
                    float *float_ptr = static_cast<float *>(member_ptr);
                    float float_value = *float_ptr;
                    TypedValue typed_result(static_cast<double>(float_value),
                                            InferredType(TYPE_FLOAT, "float"));
                    typed_result.is_numeric_result = true;
                    typed_result.is_float_result = true;
                    evaluator.set_last_typed_result(typed_result);
                    {
                        char dbg_buf[512];
                        snprintf(dbg_buf, sizeof(dbg_buf),
                                 "[ARROW_OP] Read float value: %f from 0x%lx",
                                 float_value,
                                 reinterpret_cast<uintptr_t>(member_ptr));
                        debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                    }
                    return 0; // dummy value
                } else if (member_type == TYPE_DOUBLE) {
                    double *double_ptr = static_cast<double *>(member_ptr);
                    double double_value = *double_ptr;
                    TypedValue typed_result(
                        static_cast<int64_t>(0),
                        InferredType(TYPE_DOUBLE, "double"));
                    typed_result.double_value = double_value;
                    typed_result.is_numeric_result = true;
                    evaluator.set_last_typed_result(typed_result);
                    {
                        char dbg_buf[512];
                        snprintf(dbg_buf, sizeof(dbg_buf),
                                 "[ARROW_OP] Read double value: %f from 0x%lx",
                                 double_value,
                                 reinterpret_cast<uintptr_t>(member_ptr));
                        debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                    }
                    return 0; // dummy value
                } else if (member_type == TYPE_STRING) {
                    const char **str_ptr =
                        static_cast<const char **>(member_ptr);
                    std::string str_val = (*str_ptr != nullptr) ? *str_ptr : "";
                    TypedValue typed_result(
                        str_val, InferredType(TYPE_STRING, "string"));
                    evaluator.set_last_typed_result(typed_result);
                    {
                        char dbg_buf[512];
                        snprintf(
                            dbg_buf, sizeof(dbg_buf),
                            "[ARROW_OP] Read string value: '%s' from 0x%lx",
                            str_val.c_str(),
                            reinterpret_cast<uintptr_t>(member_ptr));
                        debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                    }
                    return 0; // dummy value
                } else {
                    throw std::runtime_error(
                        "Unsupported member type in arrow operator");
                }
            }
        }

        // 通常のポインタの場合（Variable*へのポインタ）
        struct_var = reinterpret_cast<Variable *>(ptr_value);
    }

variable_access:
    if (!struct_var) {
        throw std::runtime_error("Invalid pointer in arrow operator");
    }

    auto member_it = struct_var->struct_members.find(member_name);
    if (member_it != struct_var->struct_members.end()) {
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "[ARROW_OP] struct_var=%p member='%s' value=%lld "
                     "is_assigned=%d",
                     static_cast<void *>(struct_var), member_name.c_str(),
                     member_it->second.value,
                     member_it->second.is_assigned ? 1 : 0);
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
    } else {
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "[ARROW_OP] struct_var=%p member='%s' not found",
                     static_cast<void *>(struct_var), member_name.c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
    }

    // 構造体型またはInterface型をチェック
    if (struct_var->type != TYPE_STRUCT && struct_var->type != TYPE_INTERFACE) {
        throw std::runtime_error(
            "Arrow operator requires struct or interface pointer");
    }

    // メンバーを取得
    Variable member_var = get_struct_member_func(*struct_var, member_name);

    debug_msg(DebugMsgId::GENERIC_DEBUG,
              "[ARROW_OP] member_var retrieved: type=%d, value=%lld, ");

    if (member_var.type == TYPE_STRING) {
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "[ARROW_OP] STRING member found: str_value='%s'",
                     member_var.str_value.c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }

        TypedValue typed_result(static_cast<int64_t>(0),
                                InferredType(TYPE_STRING, "string"));
        typed_result.string_value = member_var.str_value;
        typed_result.is_numeric_result = false;
        // last_typed_result_に設定
        evaluator.set_last_typed_result(typed_result);

        {
            char dbg_buf[512];
            snprintf(
                dbg_buf, sizeof(dbg_buf),
                "[ARROW_OP] set_last_typed_result called with string: '%s'",
                typed_result.string_value.c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
        return 0;
    } else if (member_var.type == TYPE_POINTER) {
        // ポインタメンバの場合はそのまま値を返す
        return member_var.value;
    } else if (member_var.type == TYPE_STRUCT ||
               member_var.type == TYPE_INTERFACE) {
        // 構造体メンバの場合は、その構造体へのポインタを返す
        // struct_membersから実際の変数へのポインタを取得
        auto member_it = struct_var->struct_members.find(member_name);
        if (member_it != struct_var->struct_members.end()) {
            return reinterpret_cast<int64_t>(&member_it->second);
        }
        // fallback: member_varのアドレスを返す(コピーなので注意)
        throw std::runtime_error(
            "Cannot get address of temporary struct member");
    } else if (member_var.type == TYPE_FLOAT) {
        // float の場合
        TypedValue typed_result(static_cast<double>(member_var.float_value),
                                InferredType(TYPE_FLOAT, "float"));
        typed_result.is_numeric_result = true;
        typed_result.is_float_result = true;
        evaluator.set_last_typed_result(typed_result);
        return 0; // dummy value
    } else if (member_var.type == TYPE_DOUBLE) {
        // double の場合
        {
            char dbg_buf[512];
            snprintf(
                dbg_buf, sizeof(dbg_buf),
                "[ARROW_OP] Reading double member: member_var.double_value=%f",
                member_var.double_value);
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
        // double コンストラクタを使用（is_float_result が自動的に true になる）
        TypedValue typed_result(member_var.double_value,
                                InferredType(TYPE_DOUBLE, "double"));
        evaluator.set_last_typed_result(typed_result);
        return 0; // dummy value
    } else if (member_var.type == TYPE_QUAD) {
        // quad の場合
        // long double コンストラクタを使用（is_float_result が自動的に true
        // になる）
        TypedValue typed_result(member_var.quad_value,
                                InferredType(TYPE_QUAD, "quad"));
        evaluator.set_last_typed_result(typed_result);
        return 0; // dummy value
    } else {
        // INT, BOOL などの通常の型の場合も last_typed_result を設定
        TypedValue typed_result(
            member_var.value,
            InferredType(member_var.type,
                         type_info_to_string(member_var.type)));
        evaluator.set_last_typed_result(typed_result);
        return member_var.value;
    }
}

int64_t evaluate_member_array_access(
    const ASTNode *node, Interpreter &interpreter,
    std::function<int64_t(const ASTNode *)> evaluate_expression_func,
    std::function<Variable(const Variable &, const std::string &)>
        get_struct_member_func) {
    // メンバの配列アクセス: obj.member[index] または func().member[index]
    std::string obj_name;
    Variable base_struct;
    bool is_function_call = false;

    if (node->left->node_type == ASTNodeType::AST_VARIABLE ||
        node->left->node_type == ASTNodeType::AST_IDENTIFIER) {
        obj_name = node->left->name;
    } else if (node->left->node_type == ASTNodeType::AST_FUNC_CALL) {
        // 関数呼び出し結果でのメンバー配列アクセス: func().member[index]
        is_function_call = true;
        debug_msg(DebugMsgId::EXPR_EVAL_START,
                  "Function call member array access");

        try {
            evaluate_expression_func(node->left.get());
            throw std::runtime_error(
                "Function did not return a struct for member array access");
        } catch (const ReturnException &ret_ex) {
            if (ret_ex.is_struct_array && ret_ex.struct_array_3d.size() > 0) {
                throw std::runtime_error("Struct array function return member "
                                         "array access not yet supported");
            } else {
                base_struct = ret_ex.struct_value;
                obj_name = "func_result"; // 仮の名前
            }
        }
    } else {
        throw std::runtime_error(
            "Invalid object reference in member array access");
    }

    std::string member_name = node->name;

    // インデックスを評価（多次元対応）
    std::vector<int64_t> indices;
    if (node->right) {
        // 1次元の場合（従来通り）
        int64_t index = evaluate_expression_func(node->right.get());
        indices.push_back(index);
    } else if (!node->arguments.empty()) {
        // 多次元の場合
        for (const auto &arg : node->arguments) {
            int64_t index = evaluate_expression_func(arg.get());
            indices.push_back(index);
        }
    } else {
        throw std::runtime_error("No indices found for array access");
    }

    // 構造体メンバー変数を取得
    Variable member_var_copy; // 関数呼び出しの場合用のコピー
    Variable *member_var;

    if (is_function_call) {
        // 関数戻り値からメンバーを取得
        member_var_copy = get_struct_member_func(base_struct, member_name);
        member_var = &member_var_copy;
    } else {
        member_var = interpreter.get_struct_member(obj_name, member_name);
        if (!member_var) {
            throw std::runtime_error("Struct member not found: " + member_name);
        }
    }

    // 多次元配列の場合
    if (member_var->is_multidimensional && indices.size() > 1) {
        if (is_function_call) {
            // 関数戻り値の場合は直接配列要素を取得
            if (!member_var->is_array || member_var->array_values.empty()) {
                throw std::runtime_error(
                    "Member is not a valid array for multi-dimensional access");
            }
            // 多次元インデックス計算（簡易版）
            int64_t flat_index = indices[0];
            if (indices.size() > 1 && member_var->is_multidimensional) {
                // 簡易的な多次元計算（正確には別の実装が必要）
                flat_index = indices[0] * 10 + indices[1]; // 仮の計算
            }
            if (flat_index >= 0 &&
                flat_index < (int64_t)member_var->array_values.size()) {
                return member_var->array_values[flat_index];
            } else {
                throw std::runtime_error("Array index out of bounds in "
                                         "function member array access");
            }
        } else {
            return interpreter.getMultidimensionalArrayElement(*member_var,
                                                               indices);
        }
    }

    // 1次元配列の場合
    int64_t index = indices[0];
    if (is_function_call) {
        // 関数戻り値の場合
        if (!member_var->is_array || member_var->array_values.empty()) {
            throw std::runtime_error("Member is not a valid array");
        }
        if (index >= 0 && index < (int64_t)member_var->array_values.size()) {
            return member_var->array_values[index];
        } else {
            throw std::runtime_error(
                "Array index out of bounds in function member array access");
        }
    } else {
        return interpreter.get_struct_member_array_element(
            obj_name, member_name, static_cast<int>(index));
    }
}

int64_t evaluate_enum_access(const ASTNode *node, Interpreter &interpreter) {
    // enum値アクセス (EnumName::member)
    EnumManager *enum_manager = interpreter.get_enum_manager();
    int64_t enum_value;

    std::string enum_name = node->enum_name;
    std::string original_enum_name = enum_name; // デバッグ用

    // ジェネリック型の場合（Option<int>）、インスタンス化された名前に変換
    // Option<int> -> Option_int
    if (enum_name.find('<') != std::string::npos) {
        // 型名の正規化（< > , を _ に置換）
        std::string instantiated_name;
        bool in_type_args = false;

        for (char c : enum_name) {
            if (c == '<') {
                in_type_args = true;
                instantiated_name += '_';
            } else if (c == '>') {
                in_type_args = false;
                // 末尾の _ は追加しない
            } else if (c == ',' || c == ' ') {
                if (in_type_args) {
                    instantiated_name += '_';
                }
            } else if (c == '*') {
                instantiated_name += "_ptr";
            } else if (c == '[') {
                instantiated_name += "_array";
            } else if (c == ']') {
                // 配列サイズは既に処理済み
            } else {
                instantiated_name += c;
            }
        }

        enum_name = instantiated_name;

        if (interpreter.is_debug_mode()) {
            std::cerr << "[ENUM_ACCESS] Mangled: " << original_enum_name
                      << " -> " << enum_name << std::endl;
        }
    }

    // typedef名を実際のenum名に解決
    std::string resolved_enum_name =
        interpreter.get_type_manager()->resolve_typedef(enum_name);

    if (interpreter.is_debug_mode()) {
        std::cerr << "[ENUM_ACCESS] Resolved typedef: " << enum_name << " -> "
                  << resolved_enum_name << std::endl;
        std::cerr << "[ENUM_ACCESS] Looking for: " << resolved_enum_name
                  << "::" << node->enum_member << std::endl;
    }

    if (enum_manager->get_enum_value(resolved_enum_name, node->enum_member,
                                     enum_value)) {
        debug_msg(DebugMsgId::EXPR_EVAL_NUMBER, enum_value);
        return enum_value;
    }

    // v0.11.1: マングリングされた名前で見つからない場合、元の形式も試す
    // （ジェネリックenumはOption<int>という形式で登録されている可能性がある）
    if (original_enum_name != resolved_enum_name) {
        if (interpreter.is_debug_mode()) {
            std::cerr << "[ENUM_ACCESS] Trying original name: "
                      << original_enum_name << "::" << node->enum_member
                      << std::endl;
        }
        if (enum_manager->get_enum_value(original_enum_name, node->enum_member,
                                         enum_value)) {
            debug_msg(DebugMsgId::EXPR_EVAL_NUMBER, enum_value);
            return enum_value;
        }
    }

    std::string error_message =
        "Undefined enum value: " + node->enum_name + "::" + node->enum_member;
    throw std::runtime_error(error_message);
}

// v0.11.0: enum値の構築 (EnumName::member(value))
int64_t evaluate_enum_construct(const ASTNode *node, Interpreter &interpreter) {
    // enum値の構築: Option<int>::Some(42)

    std::string enum_name = node->enum_name;
    std::string original_enum_name = enum_name; // デバッグ用に元の名前を保存
    std::vector<std::string> type_arguments;
    std::string base_enum_name;

    // ジェネリック型の場合（Option<int>）、インスタンス化が必要
    if (enum_name.find('<') != std::string::npos) {
        // 基底名と型引数を抽出
        size_t lt_pos = enum_name.find('<');
        base_enum_name = enum_name.substr(0, lt_pos);

        // 型引数を抽出（簡易版: カンマで分割）
        size_t start = lt_pos + 1;
        size_t end = enum_name.find_last_of('>');
        if (end != std::string::npos && end > start) {
            std::string args_str = enum_name.substr(start, end - start);

            // カンマで分割（ネストした<>は考慮しない簡易版）
            std::string current_arg;
            for (char c : args_str) {
                if (c == ',') {
                    if (!current_arg.empty()) {
                        // 空白を削除
                        current_arg.erase(0,
                                          current_arg.find_first_not_of(" \t"));
                        current_arg.erase(current_arg.find_last_not_of(" \t") +
                                          1);
                        type_arguments.push_back(current_arg);
                        current_arg.clear();
                    }
                } else {
                    current_arg += c;
                }
            }
            if (!current_arg.empty()) {
                current_arg.erase(0, current_arg.find_first_not_of(" \t"));
                current_arg.erase(current_arg.find_last_not_of(" \t") + 1);
                type_arguments.push_back(current_arg);
            }
        }

        // インスタンス化された型名を生成
        std::string instantiated_name = base_enum_name;
        for (const auto &arg : type_arguments) {
            // 型名の正規化を適用
            std::string normalized_arg = arg;
            std::string temp;
            for (char c : normalized_arg) {
                if (c == '*') {
                    temp += "_ptr";
                } else if (c == '[') {
                    temp += "_array";
                } else if (c == ']') {
                    // skip
                } else if (c == ' ') {
                    // skip
                } else {
                    temp += c;
                }
            }
            instantiated_name += "_" + temp;
        }

        enum_name = instantiated_name;

        // ジェネリックenumのインスタンス化を試みる
        // TODO:
        // Parser経由でインスタンス化すべきだが、現時点ではinterpreterから直接アクセスできない
        // この時点でインスタンス化されていない場合はエラーになる
    }

    // typedef名を実際のenum名に解決
    std::string resolved_enum_name =
        interpreter.get_type_manager()->resolve_typedef(enum_name);

    // enum定義を取得
    EnumManager *enum_manager = interpreter.get_enum_manager();
    const EnumDefinition *enum_def =
        enum_manager->get_enum_definition(resolved_enum_name);

    // v0.11.1:
    // マングリングされた名前で見つからない場合、元の形式（角括弧）も試す
    if (!enum_def && original_enum_name != resolved_enum_name) {
        if (interpreter.is_debug_mode()) {
            std::cerr << "[ENUM_CONSTRUCT] Trying original name: "
                      << original_enum_name << std::endl;
        }
        enum_def = enum_manager->get_enum_definition(original_enum_name);
        if (enum_def) {
            // 見つかった場合、resolved_enum_nameを更新
            resolved_enum_name = original_enum_name;
        }
    }

    if (!enum_def) {
        std::string error_message = "Undefined enum: " + original_enum_name +
                                    " (resolved to: " + resolved_enum_name +
                                    ")";
        if (!type_arguments.empty()) {
            error_message += "\nHint: Generic enum '" + base_enum_name +
                             "' needs to be instantiated before use.";
            error_message += "\nTry using it in a type context first (e.g., "
                             "variable declaration).";
        }
        throw std::runtime_error(error_message);
    }

    // メンバーを検索
    const EnumMember *member = enum_def->find_member(node->enum_member);
    if (!member) {
        std::string error_message =
            "Undefined enum member: " + node->enum_name +
            "::" + node->enum_member;
        throw std::runtime_error(error_message);
    }

    // 関連値の有無をチェック
    if (!member->has_associated_value) {
        std::string error_message = "Enum member " + node->enum_name +
                                    "::" + node->enum_member +
                                    " does not have an associated value";
        throw std::runtime_error(error_message);
    }

    // 引数を評価（現時点では単純な値のみサポート）
    if (node->arguments.empty()) {
        std::string error_message = "Enum constructor " + node->enum_name +
                                    "::" + node->enum_member +
                                    " requires an argument";
        throw std::runtime_error(error_message);
    }

    // 関連値を評価して返す
    // Option<int>::Some(42) の場合、42 を返す
    int64_t arg_value = interpreter.eval_expression(node->arguments[0].get());

    // デバッグ出力
    debug_msg(DebugMsgId::EXPR_EVAL_NUMBER, arg_value);

    // v0.11.0: 簡易実装として関連値を直接返す
    // enum型変数への代入時は、variable_declaration.cppで特別処理される
    // TODO: 将来的にはenum値オブジェクトを返し、型に応じて自動変換する
    return arg_value;
}

} // namespace SpecialAccessHelpers
