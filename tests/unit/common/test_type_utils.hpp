#pragma once
#include "../framework/test_framework.h"
#include "../../../src/common/ast.h"

inline void test_type_info_to_string_basic_types() {
    ASSERT_STREQ("void", type_info_to_string(TYPE_VOID));
    ASSERT_STREQ("tiny", type_info_to_string(TYPE_TINY));
    ASSERT_STREQ("short", type_info_to_string(TYPE_SHORT));
    ASSERT_STREQ("int", type_info_to_string(TYPE_INT));
    ASSERT_STREQ("long", type_info_to_string(TYPE_LONG));
    ASSERT_STREQ("string", type_info_to_string(TYPE_STRING));
    ASSERT_STREQ("bool", type_info_to_string(TYPE_BOOL));
}

inline void test_type_info_to_string_array_types() {
    ASSERT_STREQ("tiny[]", type_info_to_string(static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_TINY)));
    ASSERT_STREQ("short[]", type_info_to_string(static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_SHORT)));
    ASSERT_STREQ("int[]", type_info_to_string(static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_INT)));
    ASSERT_STREQ("long[]", type_info_to_string(static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_LONG)));
    ASSERT_STREQ("string[]", type_info_to_string(static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING)));
    ASSERT_STREQ("bool[]", type_info_to_string(static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_BOOL)));
}

inline void test_type_info_to_string_unknown_type() {
    ASSERT_STREQ("unknown", type_info_to_string(static_cast<TypeInfo>(50))); // TYPE_ARRAY_BASEより小さい不明な値
}

inline void test_bool_to_string() {
    ASSERT_STREQ("true", bool_to_string(true));
    ASSERT_STREQ("false", bool_to_string(false));
}

inline void register_type_utils_tests() {
    test_runner.add_test("common", "type_info_to_string basic types", test_type_info_to_string_basic_types);
    test_runner.add_test("common", "type_info_to_string array types", test_type_info_to_string_array_types);
    test_runner.add_test("common", "type_info_to_string unknown type", test_type_info_to_string_unknown_type);
    test_runner.add_test("common", "bool_to_string", test_bool_to_string);
}
