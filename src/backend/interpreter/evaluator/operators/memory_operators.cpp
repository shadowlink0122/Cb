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

// sizeof演算子の評価
int64_t Interpreter::evaluate_sizeof_expression(const ASTNode *node) {
    size_t result_size = 0;

    if (!node->sizeof_type_name.empty()) {
        // sizeof(Type)
        result_size = get_type_size(node->sizeof_type_name, this);

        if (debug_mode) {
            std::cerr << "[sizeof] Type: " << node->sizeof_type_name
                      << ", size=" << result_size << std::endl;
        }
    } else if (node->sizeof_expr) {
        // sizeof(expr) - 式の型からサイズを取得
        // TODO: 式の型情報を正しく取得する必要がある
        // 今は仮としてint64_tのサイズを返す
        result_size = sizeof(int64_t);

        if (debug_mode) {
            std::cerr << "[sizeof] Expression, size=" << result_size
                      << std::endl;
        }
    }

    return static_cast<int64_t>(result_size);
}
