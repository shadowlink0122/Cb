#ifndef TYPE_HELPERS_H
#define TYPE_HELPERS_H

#include "ast.h"

/**
 * @brief 型チェックヘルパー関数群
 * 
 * TypedValueの型チェックを簡潔に行うためのヘルパー関数。
 * DRY原則に従い、重複する型チェックロジックを一元化。
 */
namespace TypeHelpers {

// ===========================
// 基本型チェック
// ===========================

/**
 * @brief 整数型かどうかを判定
 */
inline bool isInteger(const TypedValue& val) {
    return val.type == TYPE_TINY || val.type == TYPE_SHORT || 
           val.type == TYPE_INT || val.type == TYPE_LONG;
}

/**
 * @brief 浮動小数点型かどうかを判定
 */
inline bool isFloating(const TypedValue& val) {
    return val.type == TYPE_FLOAT || val.type == TYPE_DOUBLE;
}

/**
 * @brief 数値型かどうかを判定（整数または浮動小数点）
 */
inline bool isNumeric(const TypedValue& val) {
    return isInteger(val) || isFloating(val);
}

/**
 * @brief 符号なし整数型かどうかを判定
 */
inline bool isUnsigned(const TypedValue& val) {
    return val.type == TYPE_UNSIGNED;
}

/**
 * @brief ポインタ型かどうかを判定
 */
inline bool isPointer(const TypedValue& val) {
    return val.type == TYPE_POINTER;
}

/**
 * @brief 参照型かどうかを判定
 */
inline bool isReference(const TypedValue& val) {
    return val.type == TYPE_REFERENCE;
}

/**
 * @brief 配列型かどうかを判定
 */
inline bool isArray(const TypedValue& val) {
    return val.type == TYPE_ARRAY;
}

/**
 * @brief 構造体型かどうかを判定
 */
inline bool isStruct(const TypedValue& val) {
    return val.type == TYPE_STRUCT;
}

/**
 * @brief Union型かどうかを判定
 */
inline bool isUnion(const TypedValue& val) {
    return val.type == TYPE_UNION;
}

/**
 * @brief 関数ポインタ型かどうかを判定
 */
inline bool isFunctionPointer(const TypedValue& val) {
    return val.type == TYPE_FUNCTION_POINTER;
}

/**
 * @brief 文字列型かどうかを判定
 */
inline bool isString(const TypedValue& val) {
    return val.type == TYPE_STRING;
}

/**
 * @brief ブーリアン型かどうかを判定
 */
inline bool isBoolean(const TypedValue& val) {
    return val.type == TYPE_BOOL;
}

/**
 * @brief void型かどうかを判定
 */
inline bool isVoid(const TypedValue& val) {
    return val.type == TYPE_VOID;
}

// ===========================
// 複合型チェック
// ===========================

/**
 * @brief ポインタまたは参照型かどうかを判定
 */
inline bool isPointerOrReference(const TypedValue& val) {
    return isPointer(val) || isReference(val);
}

/**
 * @brief 間接参照可能な型かどうかを判定
 */
inline bool isDereferenceable(const TypedValue& val) {
    return isPointer(val) || isArray(val);
}

/**
 * @brief 集約型かどうかを判定（配列、構造体、Union）
 */
inline bool isAggregate(const TypedValue& val) {
    return isArray(val) || isStruct(val) || isUnion(val);
}

/**
 * @brief 呼び出し可能な型かどうかを判定
 */
inline bool isCallable(const TypedValue& val) {
    return isFunctionPointer(val);
}

// ===========================
// 型名取得
// ===========================

/**
 * @brief 型名を文字列で取得
 */
inline const char* getTypeName(ValueType type) {
    switch (type) {
        case TYPE_TINY: return "tiny";
        case TYPE_SHORT: return "short";
        case TYPE_INT: return "int";
        case TYPE_LONG: return "long";
        case TYPE_FLOAT: return "float";
        case TYPE_DOUBLE: return "double";
        case TYPE_UNSIGNED: return "unsigned";
        case TYPE_BOOL: return "bool";
        case TYPE_STRING: return "string";
        case TYPE_POINTER: return "pointer";
        case TYPE_REFERENCE: return "reference";
        case TYPE_ARRAY: return "array";
        case TYPE_STRUCT: return "struct";
        case TYPE_UNION: return "union";
        case TYPE_FUNCTION_POINTER: return "function_pointer";
        case TYPE_VOID: return "void";
        default: return "unknown";
    }
}

/**
 * @brief TypedValueの型名を取得
 */
inline const char* getTypeName(const TypedValue& val) {
    return getTypeName(val.type);
}

// ===========================
// 型の互換性チェック
// ===========================

/**
 * @brief 2つの型が同じカテゴリかどうかを判定
 */
inline bool isSameCategory(const TypedValue& a, const TypedValue& b) {
    if (isInteger(a) && isInteger(b)) return true;
    if (isFloating(a) && isFloating(b)) return true;
    if (isPointer(a) && isPointer(b)) return true;
    if (isArray(a) && isArray(b)) return true;
    if (isStruct(a) && isStruct(b)) return true;
    return a.type == b.type;
}

/**
 * @brief 暗黙の型変換が可能かどうかを判定
 */
inline bool isImplicitlyConvertible(const TypedValue& from, const TypedValue& to) {
    // 同じ型は常に変換可能
    if (from.type == to.type) return true;
    
    // 数値型間の変換
    if (isNumeric(from) && isNumeric(to)) return true;
    
    // 配列からポインタへの変換
    if (isArray(from) && isPointer(to)) return true;
    
    return false;
}

} // namespace TypeHelpers

#endif // TYPE_HELPERS_H
