#include "managers/type_manager.h"
#include "../../../common/debug_messages.h"
#include "core/interpreter.h"
#include "managers/enum_manager.h"
#include "services/expression_service.h" // DRY効率化: 統一式評価サービス
#include <limits>
#include <stdexcept>

void TypeManager::register_typedef(const std::string &name,
                                   const std::string &type_name) {
    // 重複チェック
    if (interpreter_->typedef_map.find(name) !=
        interpreter_->typedef_map.end()) {
        error_msg(DebugMsgId::VAR_REDECLARE_ERROR, name.c_str());
        throw std::runtime_error("Typedef redefinition error: " + name);
    }
    interpreter_->typedef_map[name] = type_name;
}

std::string TypeManager::resolve_typedef(const std::string &type_name) {
    auto it = interpreter_->typedef_map.find(type_name);
    if (it != interpreter_->typedef_map.end()) {
        // さらに別のtypedefの可能性があるので再帰的に解決
        return resolve_typedef(it->second);
    }

    // ユニオン型の場合の処理
    auto union_it = union_definitions_.find(type_name);
    if (union_it != union_definitions_.end()) {
        const UnionDefinition &union_def = union_it->second;

        // 単一の基本型の場合は、その型を返す
        if (union_def.allowed_types.size() == 1 &&
            !union_def.has_literal_values && !union_def.has_custom_types &&
            !union_def.has_array_types) {
            TypeInfo base_type = *union_def.allowed_types.begin();
            switch (base_type) {
            case TYPE_INT:
                return "int";
            case TYPE_LONG:
                return "long";
            case TYPE_SHORT:
                return "short";
            case TYPE_TINY:
                return "tiny";
            case TYPE_BOOL:
                return "bool";
            case TYPE_STRING:
                return "string";
            case TYPE_CHAR:
                return "char";
            case TYPE_VOID:
                return "void";
            case TYPE_FLOAT:
                return "float";
            case TYPE_DOUBLE:
                return "double";
            case TYPE_BIG:
                return "big";
            case TYPE_QUAD:
                return "quad";
            default:
                break;
            }
        }

        // 複合ユニオン型の場合はユニオン型名を返す
        return "union " + type_name;
    }

    return type_name; // typedef aliasでない場合はそのまま返す
}

std::string
TypeManager::resolve_typedef_one_level(const std::string &type_name) {
    auto it = interpreter_->typedef_map.find(type_name);
    if (it != interpreter_->typedef_map.end()) {
        return it->second; // 1段階のみ解決して返す
    }
    return type_name; // typedef aliasでない場合はそのまま返す
}

TypeInfo TypeManager::string_to_type_info(const std::string &type_str) {
    std::string resolved = resolve_typedef(type_str);

    if (resolved.rfind("unsigned ", 0) == 0) {
        resolved = resolved.substr(9);
    }

    if (resolved == "int")
        return TYPE_INT;
    if (resolved == "long")
        return TYPE_LONG;
    if (resolved == "short")
        return TYPE_SHORT;
    if (resolved == "tiny")
        return TYPE_TINY;
    if (resolved == "bool")
        return TYPE_BOOL;
    if (resolved == "string")
        return TYPE_STRING;
    if (resolved == "char")
        return TYPE_CHAR;
    if (resolved == "float")
        return TYPE_FLOAT;
    if (resolved == "double")
        return TYPE_DOUBLE;
    if (resolved == "big")
        return TYPE_BIG;
    if (resolved == "quad")
        return TYPE_QUAD;
    if (resolved == "void")
        return TYPE_VOID;

    // struct型チェック
    if (resolved.substr(0, 7) == "struct ") {
        return TYPE_STRUCT;
    }

    // enum型チェック
    if (resolved.substr(0, 5) == "enum ") {
        return TYPE_ENUM;
    }

    // typedef済みstructやenumの名前だけの場合もチェック
    if (interpreter_->find_struct_definition(resolved) != nullptr) {
        return TYPE_STRUCT;
    }

    if (interpreter_->get_enum_manager()->enum_exists(resolved)) {
        return TYPE_ENUM;
    }

    return TYPE_UNKNOWN;
}

void TypeManager::check_type_range(TypeInfo type, int64_t value,
                                   const std::string &var_name,
                                   bool is_unsigned) {
    // DRY効率化: 統一式評価サービスを使用した安全な型範囲チェック
    interpreter_->get_expression_service()->evaluate_safe(
        nullptr, "type_range_check_" + var_name, [&](const std::string &error) {
            int64_t min_allowed = 0;
            int64_t max_allowed = 0;
            bool has_range = true;

            switch (type) {
            case TYPE_TINY:
                if (is_unsigned) {
                    min_allowed = 0;
                    max_allowed = 255;
                } else {
                    min_allowed = -128;
                    max_allowed = 127;
                }
                break;
            case TYPE_SHORT:
                if (is_unsigned) {
                    min_allowed = 0;
                    max_allowed = 65535;
                } else {
                    min_allowed = -32768;
                    max_allowed = 32767;
                }
                break;
            case TYPE_INT:
                if (is_unsigned) {
                    min_allowed = 0;
                    max_allowed = static_cast<int64_t>(
                        std::numeric_limits<uint32_t>::max());
                } else {
                    min_allowed = static_cast<int64_t>(INT32_MIN);
                    max_allowed = static_cast<int64_t>(INT32_MAX);
                }
                break;
            case TYPE_CHAR:
                if (is_unsigned) {
                    min_allowed = 0;
                    max_allowed = 255;
                } else {
                    min_allowed = -128;
                    max_allowed = 127;
                }
                break;
            case TYPE_LONG:
                if (is_unsigned) {
                    min_allowed = 0;
                    max_allowed = std::numeric_limits<int64_t>::max();
                } else {
                    min_allowed = std::numeric_limits<int64_t>::min();
                    max_allowed = std::numeric_limits<int64_t>::max();
                }
                break;
            default:
                has_range = false;
                break; // 他の型はチェックしない
            }

            if (!has_range) {
                return;
            }

            if (value < min_allowed || value > max_allowed) {
                error_msg(DebugMsgId::TYPE_RANGE_ERROR, var_name.c_str());
                throw std::runtime_error("Value out of range for type");
            }
        });
}

// union typedef処理
void TypeManager::register_union_typedef(const std::string &name,
                                         const UnionDefinition &union_def) {
    extern bool debug_mode;
    if (debug_mode) {
        debug_print("REGISTER_UNION_DEBUG: Registering union typedef '%s'\n",
                    name.c_str());
        debug_print(
            "REGISTER_UNION_DEBUG: has_literal_values=%d, has_type_values=%d, "
            "has_custom_types=%d, has_array_types=%d\n",
            union_def.has_literal_values, union_def.has_type_values,
            union_def.has_custom_types, union_def.has_array_types);
        debug_print(
            "REGISTER_UNION_DEBUG: allowed_values.size()=%zu, "
            "allowed_types.size()=%zu, allowed_custom_types.size()=%zu, "
            "allowed_array_types.size()=%zu\n",
            union_def.allowed_values.size(), union_def.allowed_types.size(),
            union_def.allowed_custom_types.size(),
            union_def.allowed_array_types.size());

        if (union_def.has_type_values) {
            debug_print("REGISTER_UNION_DEBUG: Allowed types: ");
            for (const auto &type : union_def.allowed_types) {
                debug_print("%d ", static_cast<int>(type));
            }
            debug_print("\n");
        }

        if (union_def.has_custom_types) {
            debug_print("REGISTER_UNION_DEBUG: Allowed custom types: ");
            for (const auto &custom_type : union_def.allowed_custom_types) {
                debug_print("%s ", custom_type.c_str());
            }
            debug_print("\n");
        }

        if (union_def.has_array_types) {
            debug_print("REGISTER_UNION_DEBUG: Allowed array types: ");
            for (const auto &array_type : union_def.allowed_array_types) {
                debug_print("%s ", array_type.c_str());
            }
            debug_print("\n");
        }
    }

    // 重複チェック
    if (union_definitions_.find(name) != union_definitions_.end()) {
        throw std::runtime_error("Union typedef redefinition error: " + name);
    }
    if (interpreter_->typedef_map.find(name) !=
        interpreter_->typedef_map.end()) {
        throw std::runtime_error("Typedef name already exists: " + name);
    }

    // union_definitions_[name] = union_def; //
    // この方法はデフォルトコンストラクタを呼び出すため問題
    union_definitions_.insert(std::make_pair(
        name, union_def)); // 直接挿入でデフォルトコンストラクタを回避

    // Note: ユニオン型はtypedef_mapに登録しない（別の管理システムで管理）
}

bool TypeManager::is_union_type(const std::string &type_name) {
    return union_definitions_.find(type_name) != union_definitions_.end();
}

std::string TypeManager::get_union_lookup_name(const Variable &variable) const {
    if (variable.is_pointer && !variable.pointer_base_type_name.empty()) {
        return variable.pointer_base_type_name;
    }
    return variable.type_name;
}

bool TypeManager::is_union_type(const Variable &variable) {
    std::string lookup = get_union_lookup_name(variable);
    if (lookup.empty()) {
        return false;
    }
    return is_union_type(lookup);
}

bool TypeManager::is_value_allowed_for_union(const std::string &type_name,
                                             const std::string &str_value) {
    auto it = union_definitions_.find(type_name);
    if (it == union_definitions_.end()) {
        return false;
    }

    const UnionDefinition &union_def = it->second;

    // リテラル値チェック
    if (union_def.has_literal_values) {
        for (const auto &allowed : union_def.allowed_values) {
            if (allowed.value_type == TYPE_STRING &&
                allowed.string_value == str_value) {
                return true;
            }
        }
        // リテラル値ユニオンで値が見つからなかった場合はfalseを返す
        return false;
    }

    // 型チェック（リテラル値ユニオンでない場合のみ）
    bool basic_type_result = union_def.is_type_allowed(TYPE_STRING);

    // カスタム型チェック（string値の場合）
    if (!basic_type_result && union_def.has_custom_types) {
        for (const auto &custom_type : union_def.allowed_custom_types) {
            // カスタム型がstring型へのtypedefかどうかをチェック
            std::string resolved_type = resolve_typedef(custom_type);
            if (resolved_type == "string") {
                return true;
            }
        }
    }

    return basic_type_result;
}

bool TypeManager::is_value_allowed_for_union(const std::string &type_name,
                                             int64_t int_value) {
    auto it = union_definitions_.find(type_name);
    if (it == union_definitions_.end()) {
        return false;
    }

    const UnionDefinition &union_def = it->second;

    extern bool debug_mode;
    if (debug_mode) {
        debug_print(
            "UNION_TYPE_DEBUG: Checking int value %lld for union type %s\n",
            int_value, type_name.c_str());
        debug_print(
            "UNION_TYPE_DEBUG: has_literal_values=%d, has_type_values=%d\n",
            union_def.has_literal_values, union_def.has_type_values);
        debug_print("UNION_TYPE_DEBUG: allowed_values.size()=%zu, "
                    "allowed_types.size()=%zu\n",
                    union_def.allowed_values.size(),
                    union_def.allowed_types.size());

        if (union_def.has_type_values) {
            debug_print("UNION_TYPE_DEBUG: Allowed types: ");
            for (const auto &type : union_def.allowed_types) {
                debug_print("%d ", static_cast<int>(type));
            }
            debug_print("\n");
        }
    }

    // リテラル値チェック
    if (union_def.has_literal_values) {
        // bool literal_found = false;  // 将来の拡張のため残すがコメントアウト
        for (const auto &allowed : union_def.allowed_values) {
            if (debug_mode) {
                debug_print("UNION_TYPE_DEBUG: Comparing with allowed value: "
                            "type=%d, int_value=%lld, bool_value=%d\n",
                            static_cast<int>(allowed.value_type),
                            allowed.int_value, allowed.bool_value);
            }
            if ((allowed.value_type == TYPE_INT ||
                 allowed.value_type == TYPE_LONG ||
                 allowed.value_type == TYPE_SHORT ||
                 allowed.value_type == TYPE_TINY ||
                 allowed.value_type == TYPE_CHAR) &&
                allowed.int_value == int_value) {
                return true;
            }
            // boolean値もチェック
            if (allowed.value_type == TYPE_BOOL &&
                allowed.bool_value == (int_value == 1)) {
                return true;
            }
        }

        // リテラル値が定義されているがマッチしなかった場合
        // 混合ユニオンでも、明示されたリテラル値でない限りfalse
        if (debug_mode) {
            debug_print("UNION_TYPE_DEBUG: Literal value %lld not found in "
                        "allowed values\n",
                        int_value);
        }

        // 値が数値で、かつint系型が許可されている場合のみ型チェックに進む
        bool has_int_types = union_def.is_type_allowed(TYPE_INT) ||
                             union_def.is_type_allowed(TYPE_LONG) ||
                             union_def.is_type_allowed(TYPE_SHORT) ||
                             union_def.is_type_allowed(TYPE_TINY) ||
                             union_def.is_type_allowed(TYPE_CHAR);

        if (has_int_types) {
            if (debug_mode) {
                debug_print("UNION_TYPE_DEBUG: Mixed union has int types, "
                            "checking if value fits\n");
            }
        } else {
            // int系の型が許可されておらず、リテラル値も一致しない場合はfalse
            if (debug_mode) {
                debug_print("UNION_TYPE_DEBUG: No int types allowed and "
                            "literal value not found, returning false\n");
            }
            return false;
        }
    }

    // 型チェック（明示的に定義された基本型のみをチェック）
    bool basic_type_result = false;
    if (union_def.has_type_values) {
        for (TypeInfo allowed_type : union_def.allowed_types) {
            if (allowed_type == TYPE_INT || allowed_type == TYPE_LONG ||
                allowed_type == TYPE_SHORT || allowed_type == TYPE_TINY ||
                allowed_type == TYPE_CHAR) {
                basic_type_result = true;
                break;
            }
        }
    }

    if (debug_mode) {
        debug_print("UNION_TYPE_DEBUG: Basic type check result = %d\n",
                    basic_type_result);
    }

    // 混合ユニオンの場合の処理
    if (union_def.has_literal_values) {
        // 基本型が明示的に許可されている場合は、基本型の値も許可
        if (basic_type_result) {
            if (debug_mode) {
                debug_print("UNION_TYPE_DEBUG: Mixed union with literals and "
                            "explicit int type - allowing int value %lld\n",
                            int_value);
            }
            return true;
        } else {
            // 基本型が明示的に許可されていない場合は、リテラル値のみ
            if (debug_mode) {
                debug_print("UNION_TYPE_DEBUG: Mixed union with literals but "
                            "no explicit int type - rejecting int value %lld\n",
                            int_value);
            }
            return false;
        }
    }

    // カスタム型チェック（int値の場合）
    if (!basic_type_result && union_def.has_custom_types) {
        if (debug_mode) {
            debug_print("UNION_CUSTOM_TYPE_DEBUG: Checking custom types for "
                        "int value %lld\n",
                        int_value);
            debug_print(
                "UNION_CUSTOM_TYPE_DEBUG: Number of custom types: %zu\n",
                union_def.allowed_custom_types.size());
        }
        for (const auto &custom_type : union_def.allowed_custom_types) {
            if (debug_mode) {
                debug_print(
                    "UNION_CUSTOM_TYPE_DEBUG: Examining custom type '%s'\n",
                    custom_type.c_str());
            }
            // カスタム型がint系の基本型へのtypedefかどうかをチェック
            std::string resolved_type = resolve_typedef(custom_type);
            if (debug_mode) {
                debug_print("UNION_CUSTOM_TYPE_DEBUG: Custom type '%s' "
                            "resolves to '%s'\n",
                            custom_type.c_str(), resolved_type.c_str());
            }
            if (resolved_type == "int" || resolved_type == "long" ||
                resolved_type == "short" || resolved_type == "tiny" ||
                resolved_type == "char") {
                if (debug_mode) {
                    debug_print("UNION_CUSTOM_TYPE_DEBUG: Custom type '%s' is "
                                "int-compatible, returning true\n",
                                custom_type.c_str());
                }
                return true;
            }
        }
        if (debug_mode) {
            debug_print("UNION_CUSTOM_TYPE_DEBUG: No int-compatible custom "
                        "types found\n");
        }
    }

    return basic_type_result;
}

bool TypeManager::is_value_allowed_for_union(const std::string &type_name,
                                             bool bool_value) {
    auto it = union_definitions_.find(type_name);
    if (it == union_definitions_.end()) {
        return false;
    }

    const UnionDefinition &union_def = it->second;

    // リテラル値チェック
    if (union_def.has_literal_values) {
        for (const auto &allowed : union_def.allowed_values) {
            if (allowed.value_type == TYPE_BOOL &&
                allowed.bool_value == bool_value) {
                return true;
            }
        }
        // リテラル値ユニオンで値が見つからなかった場合はfalseを返す
        return false;
    }

    // 型チェック（リテラル値ユニオンでない場合のみ）
    bool basic_type_result = union_def.is_type_allowed(TYPE_BOOL);

    // カスタム型チェック（bool値の場合）
    if (!basic_type_result && union_def.has_custom_types) {
        for (const auto &custom_type : union_def.allowed_custom_types) {
            // カスタム型がbool型へのtypedefかどうかをチェック
            std::string resolved_type = resolve_typedef(custom_type);
            if (resolved_type == "bool") {
                return true;
            }
        }
    }

    return basic_type_result;
}

bool TypeManager::is_custom_type_allowed_for_union(
    const std::string &union_type_name, const std::string &custom_type_name) {
    auto it = union_definitions_.find(union_type_name);
    if (it == union_definitions_.end()) {
        return false;
    }

    const UnionDefinition &union_def = it->second;

    extern bool debug_mode;
    if (debug_mode) {
        debug_print("UNION_CUSTOM_TYPE_DEBUG: Checking custom type '%s' for "
                    "union type %s\n",
                    custom_type_name.c_str(), union_type_name.c_str());
        debug_print("UNION_CUSTOM_TYPE_DEBUG: has_custom_types=%d, "
                    "allowed_custom_types.size()=%zu\n",
                    union_def.has_custom_types,
                    union_def.allowed_custom_types.size());
    }

    bool result = union_def.is_custom_type_allowed(custom_type_name);
    if (debug_mode) {
        debug_print("UNION_CUSTOM_TYPE_DEBUG: is_custom_type_allowed('%s') "
                    "returned %d\n",
                    custom_type_name.c_str(), result);
        if (union_def.has_custom_types) {
            debug_print("UNION_CUSTOM_TYPE_DEBUG: Allowed custom types are: ");
            for (const auto &allowed : union_def.allowed_custom_types) {
                debug_print("'%s' ", allowed.c_str());
            }
            debug_print("\n");
        }
    }
    return result;
}

bool TypeManager::is_array_type_allowed_for_union(
    const std::string &union_type_name, const std::string &array_type) {
    auto it = union_definitions_.find(union_type_name);
    if (it == union_definitions_.end()) {
        return false;
    }

    const UnionDefinition &union_def = it->second;

    extern bool debug_mode;
    if (debug_mode) {
        debug_print("UNION_ARRAY_TYPE_DEBUG: Checking array type '%s' for "
                    "union type %s\n",
                    array_type.c_str(), union_type_name.c_str());
        debug_print("UNION_ARRAY_TYPE_DEBUG: has_array_types=%d, "
                    "allowed_array_types.size()=%zu\n",
                    union_def.has_array_types,
                    union_def.allowed_array_types.size());
    }

    return union_def.is_array_type_allowed(array_type);
}
