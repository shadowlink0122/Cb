// Memory Management Operators - new/delete/sizeof
// v0.11.0 Phase 1a - メモリ管理機能

#include "src/backend/interpreter/core/interpreter.h"
#include "src/backend/interpreter/evaluator/core/evaluator.h"
#include <cstdlib>
#include <cstring>
#include <unordered_map>

// 型名からサイズを取得するヘルパー関数（再帰対応）
static size_t get_type_size(const std::string &type_name,
                            const Interpreter *interpreter) {
    // プリミティブ型のサイズマップ
    // Cb型定義: tiny=8bit, short=16bit, int=32bit, long=64bit
    static const std::unordered_map<std::string, size_t> type_sizes = {
        {"int", 4},                // 32bit
        {"long", 8},               // 64bit
        {"short", 2},              // 16bit
        {"tiny", 1},               // 8bit
        {"char", 1},               // 8bit
        {"bool", 1},               // 8bit
        {"float", 4},              // 32bit
        {"double", 8},             // 64bit
        {"void*", sizeof(void *)}, // ポインタサイズ（64bit環境で8）
        {"string", sizeof(void *)}, // 文字列はポインタとして扱う
        {"unsigned int", 4},        // 32bit
        {"unsigned long", 8},       // 64bit
        {"unsigned short", 2},      // 16bit
        {"unsigned tiny", 1},       // 8bit
    };

    // ポインタ型（*が含まれる）
    if (type_name.find('*') != std::string::npos) {
        return sizeof(void *);
    }

    // プリミティブ型のマップから検索
    auto it = type_sizes.find(type_name);
    if (it != type_sizes.end()) {
        return it->second;
    }

    if (!interpreter) {
        // interpreterがない場合はポインタサイズをデフォルトとして返す
        return sizeof(void *);
    }

    // typedefを解決
    std::string resolved_type = interpreter->resolve_typedef(type_name);
    if (resolved_type != type_name) {
        // typedefされた型なので、再帰的に解決
        return get_type_size(resolved_type, interpreter);
    }

    // 構造体のサイズを計算
    const StructDefinition *struct_def =
        interpreter->get_struct_definition(type_name);
    if (struct_def) {
        size_t total_size = 0;

        // 各メンバーのサイズを合計
        for (const auto &member : struct_def->members) {
            size_t member_size = 0;

            // メンバーの型に応じてサイズを決定
            if (member.is_pointer) {
                // ポインタメンバー
                member_size = sizeof(void *);
            } else {
                // 型名を決定
                std::string member_type_str;
                switch (member.type) {
                case TYPE_INT:
                    member_type_str = "int";
                    break;
                case TYPE_LONG:
                    member_type_str = "long";
                    break;
                case TYPE_SHORT:
                    member_type_str = "short";
                    break;
                case TYPE_TINY:
                    member_type_str = "tiny";
                    break;
                case TYPE_CHAR:
                    member_type_str = "char";
                    break;
                case TYPE_BOOL:
                    member_type_str = "bool";
                    break;
                case TYPE_FLOAT:
                    member_type_str = "float";
                    break;
                case TYPE_DOUBLE:
                    member_type_str = "double";
                    break;
                case TYPE_STRING:
                    member_type_str = "string";
                    break;
                case TYPE_STRUCT:
                    // ネストした構造体: type_aliasまたは推論
                    if (!member.type_alias.empty()) {
                        member_type_str = member.type_alias;
                    } else {
                        // フォールバック: ポインタサイズ
                        member_size = sizeof(void *);
                        break;
                    }
                    break;
                default:
                    // 不明な型はint扱い
                    member_type_str = "int";
                }

                // 再帰的にサイズを取得
                if (!member_type_str.empty()) {
                    member_size = get_type_size(member_type_str, interpreter);
                }
            }

            // 配列の場合はサイズを掛ける
            if (member.array_info.is_array()) {
                size_t array_total_size = 1;
                for (const auto &dim : member.array_info.dimensions) {
                    if (dim.size > 0) {
                        array_total_size *= dim.size;
                    }
                }
                member_size *= array_total_size;
            }

            total_size += member_size;
        }

        return total_size > 0 ? total_size : sizeof(void *);
    }

    // 不明な型の場合はポインタサイズを返す
    return sizeof(void *);
}

// new演算子の評価
int64_t Interpreter::evaluate_new_expression(const ASTNode *node) {
    if (node->is_array_new) {
        // new T[size]
        int64_t array_size = expression_evaluator_->evaluate_expression(
            node->new_array_size.get());
        size_t element_size = get_type_size(node->new_type_name, this);
        size_t total_size = static_cast<size_t>(array_size) * element_size;

        void *ptr = std::malloc(total_size);
        if (!ptr) {
            throw std::runtime_error("Memory allocation failed");
        }

        // メモリをゼロクリア
        std::memset(ptr, 0, total_size);

        if (debug_mode) {
            std::cerr << "[new] Allocated array: type=" << node->new_type_name
                      << ", size=" << array_size
                      << ", total_bytes=" << total_size << ", ptr=" << ptr
                      << std::endl;
        }

        return reinterpret_cast<int64_t>(ptr);
    } else {
        // new T
        // 構造体型かどうかをチェック
        const StructDefinition *struct_def =
            get_struct_definition(node->new_type_name);

        if (struct_def) {
            // 構造体の場合: Variableオブジェクトをヒープに作成
            Variable *struct_var = new Variable();
            struct_var->type = TYPE_STRUCT;
            struct_var->struct_type_name = node->new_type_name;
            struct_var->is_assigned = true;
            struct_var->is_struct = true; // 構造体フラグを設定

            // 各メンバーを初期化
            for (const auto &member : struct_def->members) {
                Variable member_var;
                member_var.type = member.type;
                member_var.is_pointer = member.is_pointer;
                member_var.is_assigned = false;
                member_var.value = 0;
                member_var.float_value = 0.0;

                // 構造体メンバーの場合、型名を設定
                if (member.type == TYPE_STRUCT && !member.type_alias.empty()) {
                    member_var.struct_type_name = member.type_alias;
                }

                // メンバーを追加
                struct_var->struct_members[member.name] = member_var;
            }

            if (debug_mode) {
                std::cerr << "[new] Allocated struct: type="
                          << node->new_type_name << ", Variable*=" << struct_var
                          << std::endl;
            }

            return reinterpret_cast<int64_t>(struct_var);
        } else {
            // プリミティブ型の場合: 生メモリを確保
            size_t type_size = get_type_size(node->new_type_name, this);
            void *ptr = std::malloc(type_size);
            if (!ptr) {
                throw std::runtime_error("Memory allocation failed");
            }

            // メモリをゼロクリア
            std::memset(ptr, 0, type_size);

            if (debug_mode) {
                std::cerr << "[new] Allocated object: type="
                          << node->new_type_name << ", size=" << type_size
                          << ", ptr=" << ptr << std::endl;
            }

            return reinterpret_cast<int64_t>(ptr);
        }
    }
}

// delete演算子の評価 (delete[]構文は廃止、統一してdelete ptr;のみ)
int64_t Interpreter::evaluate_delete_expression(const ASTNode *node) {
    int64_t ptr_value =
        expression_evaluator_->evaluate_expression(node->delete_expr.get());

    if (ptr_value == 0) {
        // nullptr削除は何もしない
        return 0;
    }

    // ポインタがVariable*（構造体）かどうかを判定
    Variable *potential_var = reinterpret_cast<Variable *>(ptr_value);

    // Variable構造体の場合、type==TYPE_STRUCTでis_struct==trueのはず
    // ただし、生ポインタを誤判定しないよう慎重に判定
    bool is_struct_variable = false;
    try {
        // メモリアクセス可能かつ、Variableの構造体フラグが立っているか
        if (potential_var->type == TYPE_STRUCT && potential_var->is_struct) {
            is_struct_variable = true;
        }
    } catch (...) {
        // アクセス違反なら生ポインタ
        is_struct_variable = false;
    }

    if (debug_mode) {
        std::cerr << "[delete] Freeing ptr="
                  << reinterpret_cast<void *>(ptr_value)
                  << " is_struct=" << (is_struct_variable ? "yes" : "no")
                  << std::endl;
    }

    if (is_struct_variable) {
        // Variable*（構造体）の場合: C++ deleteを使用
        delete potential_var;
    } else {
        // 生ポインタ（プリミティブ型）の場合: freeを使用
        void *ptr = reinterpret_cast<void *>(ptr_value);
        std::free(ptr);
    }

    return 0;
}

// Variableの型からサイズを取得するヘルパー関数
static size_t get_variable_size(const Variable *var) {
    if (!var)
        return 0;

    if (var->is_pointer) {
        return sizeof(void *);
    }

    // 配列の場合は要素型を取得
    TypeInfo element_type = var->type;
    if (var->is_array || var->is_multidimensional) {
        // array_type_infoが設定されている場合はそれを使用
        if (var->array_type_info.is_array()) {
            element_type = var->array_type_info.base_type;
        }
        // レガシー配列型（TYPE_ARRAY_BASE + 基底型）の場合
        else if (var->type >= TYPE_ARRAY_BASE) {
            element_type = static_cast<TypeInfo>(var->type - TYPE_ARRAY_BASE);
        }
    }

    // 基本的な要素サイズを取得
    size_t element_size = 0;
    switch (element_type) {
    case TYPE_INT:
        element_size = sizeof(int);
        break;
    case TYPE_LONG:
        element_size = sizeof(long);
        break;
    case TYPE_SHORT:
        element_size = sizeof(short);
        break;
    case TYPE_TINY:
        element_size = sizeof(char);
        break;
    case TYPE_CHAR:
        element_size = sizeof(char);
        break;
    case TYPE_BOOL:
        element_size = sizeof(bool);
        break;
    case TYPE_FLOAT:
        element_size = sizeof(float);
        break;
    case TYPE_DOUBLE:
        element_size = sizeof(double);
        break;
    case TYPE_QUAD:
        element_size = sizeof(long double);
        break;
    case TYPE_STRING:
        element_size = sizeof(void *);
        break;
    case TYPE_STRUCT:
        element_size = sizeof(void *);
        break;
    default:
        element_size = sizeof(int64_t);
        break;
    }

    // 配列の場合、全次元の要素数を掛け算
    if (var->is_array || var->is_multidimensional) {
        size_t total_elements = 1;

        if (var->is_multidimensional && !var->array_dimensions.empty()) {
            // 多次元配列
            for (int dim_size : var->array_dimensions) {
                if (dim_size > 0) {
                    total_elements *= dim_size;
                }
            }
        } else if (var->is_array && var->array_size > 0) {
            // 1次元配列
            total_elements = var->array_size;
        }

        return element_size * total_elements;
    }

    return element_size;
}

// TypedValueからサイズを取得するヘルパー関数
static size_t get_typed_value_size(const TypedValue &tv) {
    switch (tv.type.type_info) {
    case TYPE_INT:
        return sizeof(int);
    case TYPE_LONG:
        return sizeof(long);
    case TYPE_SHORT:
        return sizeof(short);
    case TYPE_TINY:
        return sizeof(char);
    case TYPE_CHAR:
        return sizeof(char);
    case TYPE_BOOL:
        return sizeof(bool);
    case TYPE_FLOAT:
        return sizeof(float);
    case TYPE_DOUBLE:
        return sizeof(double);
    case TYPE_QUAD:
        return sizeof(long double);
    case TYPE_STRING:
        return sizeof(void *);
    case TYPE_POINTER:
        return sizeof(void *);
    case TYPE_STRUCT:
        return sizeof(void *);
    default:
        return sizeof(int64_t);
    }
}

// sizeof演算子の評価
int64_t Interpreter::evaluate_sizeof_expression(const ASTNode *node) {
    size_t result_size = 0;

    if (debug_mode) {
        std::cerr << "[sizeof] Called! sizeof_type_name='"
                  << node->sizeof_type_name
                  << "', has_expr=" << (node->sizeof_expr != nullptr)
                  << std::endl;
    }

    if (!node->sizeof_type_name.empty()) {
        // sizeof(Type)
        result_size = get_type_size(node->sizeof_type_name, this);

        if (debug_mode) {
            std::cerr << "[sizeof] Type: " << node->sizeof_type_name
                      << ", size=" << result_size << std::endl;
        }
    } else if (node->sizeof_expr) {
        // sizeof(expr) - 式の型からサイズを取得
        const ASTNode *expr = node->sizeof_expr.get();

        // 変数の場合、その型を直接取得
        if (expr->node_type == ASTNodeType::AST_VARIABLE) {
            Variable *var = find_variable(expr->name);
            if (var) {
                result_size = get_variable_size(var);
            } else {
                // 変数が見つからない場合、型名として解釈
                result_size = get_type_size(expr->name, this);
            }
        } else {
            // 式の型を推論
            TypedValue typed_val = evaluate_typed(expr);
            result_size = get_typed_value_size(typed_val);
        }

        if (debug_mode) {
            std::cerr << "[sizeof] Expression, size=" << result_size
                      << std::endl;
        }
    }

    return static_cast<int64_t>(result_size);
}
