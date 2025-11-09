// ========================================================================
// 組み込み型の初期化
// Option<T>, Result<T, E> などの組み込みEnum型を自動登録
// ========================================================================

#include "../../../common/ast.h"
#include "../../../common/debug.h"
#include "../managers/types/enums.h"
#include "interpreter.h"

// ========================================================================
// 組み込み型の初期化メイン関数
// ========================================================================
void Interpreter::initialize_builtin_types() {

    debug_msg(DebugMsgId::GENERIC_DEBUG,
              "[BUILTIN_TYPES] Initializing builtin types...");

    // Future<T> struct型を登録（v0.12.0 async/await）
    register_builtin_struct_future();

    // Option<T> enum型を登録
    register_builtin_enum_option();

    // Result<T, E> enum型を登録
    register_builtin_enum_result();

    if (debug_mode) {
        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "[BUILTIN_TYPES] Builtin types initialization complete");
    }
}

// ========================================================================
// Future<T> struct型の登録
//
// struct Future<T> {
//     T value;
//     bool is_ready;
// }
//
// v0.12.0: async/await機能で使用される組み込み型
// ========================================================================
void Interpreter::register_builtin_struct_future() {
    if (debug_mode) {
        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "[BUILTIN_TYPES] Registering Future<T>...");
    }

    // Future<T>のStructDefinitionを作成
    StructDefinition future_def("Future");
    future_def.is_generic = true;
    future_def.type_parameters.push_back("T");

    // メンバー1: T value
    StructMember value_member;
    value_member.name = "value";
    value_member.type_alias = "T";    // 型パラメータ名
    value_member.type = TYPE_UNKNOWN; // ジェネリック型パラメータ
    value_member.is_pointer = false;
    value_member.pointer_depth = 0;
    value_member.is_private = false;
    value_member.is_default = false;
    future_def.members.push_back(value_member);

    // メンバー2: bool is_ready
    StructMember is_ready_member;
    is_ready_member.name = "is_ready";
    is_ready_member.type_alias = "bool";
    is_ready_member.type = TYPE_BOOL;
    is_ready_member.is_pointer = false;
    is_ready_member.pointer_depth = 0;
    is_ready_member.is_private = false;
    is_ready_member.is_default = false;
    future_def.members.push_back(is_ready_member);

    // struct_definitions_に登録
    struct_definitions_["Future"] = future_def;

    if (debug_mode) {
        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "[BUILTIN_TYPES] Future<T> registered successfully");
    }
}

// ========================================================================
// Option<T> enum型の登録
//
// enum Option<T> {
//     Some(T),
//     None
// }
// ========================================================================
void Interpreter::register_builtin_enum_option() {
    if (debug_mode) {
        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "[BUILTIN_TYPES] Registering Option<T>...");
    }

    EnumDefinition option_def;
    option_def.name = "Option";
    option_def.is_generic = true;
    option_def.has_associated_values = true;
    option_def.type_parameters.push_back("T");

    // Some(T) variant
    EnumMember some_member;
    some_member.name = "Some";
    some_member.value = 0;
    some_member.explicit_value = true;
    some_member.has_associated_value = true;
    some_member.associated_type_name = "T";
    option_def.members.push_back(some_member);

    // None variant
    EnumMember none_member;
    none_member.name = "None";
    none_member.value = 1;
    none_member.explicit_value = true;
    none_member.has_associated_value = false;
    option_def.members.push_back(none_member);

    // enum_manager_に登録
    enum_manager_->register_enum("Option", option_def);

    if (debug_mode) {
        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "[BUILTIN_TYPES] Option<T> registered successfully");
    }
}

// ========================================================================
// Result<T, E> enum型の登録
//
// enum Result<T, E> {
//     Ok(T),
//     Err(E)
// }
// ========================================================================
void Interpreter::register_builtin_enum_result() {
    if (debug_mode) {
        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "[BUILTIN_TYPES] Registering Result<T, E>...");
    }

    EnumDefinition result_def;
    result_def.name = "Result";
    result_def.is_generic = true;
    result_def.has_associated_values = true;
    result_def.type_parameters.push_back("T");
    result_def.type_parameters.push_back("E");

    // Ok(T) variant
    EnumMember ok_member;
    ok_member.name = "Ok";
    ok_member.value = 0;
    ok_member.explicit_value = true;
    ok_member.has_associated_value = true;
    ok_member.associated_type_name = "T";
    result_def.members.push_back(ok_member);

    // Err(E) variant
    EnumMember err_member;
    err_member.name = "Err";
    err_member.value = 1;
    err_member.explicit_value = true;
    err_member.has_associated_value = true;
    err_member.associated_type_name = "E";
    result_def.members.push_back(err_member);

    // enum_manager_に登録
    enum_manager_->register_enum("Result", result_def);

    if (debug_mode) {
        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "[BUILTIN_TYPES] Result<T, E> registered successfully");
    }
}
