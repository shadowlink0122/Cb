#ifndef TYPE_HELPERS_H
#define TYPE_HELPERS_H

#include "../backend/interpreter/core/type_inference.h"
#include "ast.h"
#include <cstddef>  // for size_t
#include <cstdint>  // for int64_t, INT64_MIN, INT64_MAX

/**
 * @brief 型チェックヘルパー関数群
 *
 * TypedValueとVariableの型チェックを簡潔に行うためのヘルパー関数。
 * DRY原則に従い、重複する型チェックロジックを一元化。
 */
namespace TypeHelpers {

// ===========================
// 基本型チェック
// ===========================

/**
 * @brief 整数型かどうかを判定
 */
inline bool isInteger(const TypedValue &val) {
    return val.type.type_info == TYPE_TINY ||
           val.type.type_info == TYPE_SHORT || val.type.type_info == TYPE_INT ||
           val.type.type_info == TYPE_LONG;
}

/**
 * @brief 浮動小数点型かどうかを判定
 */
inline bool isFloating(const TypedValue &val) {
    return val.type.type_info == TYPE_FLOAT ||
           val.type.type_info == TYPE_DOUBLE;
}

/**
 * @brief 数値型かどうかを判定（整数または浮動小数点）
 */
inline bool isNumeric(const TypedValue &val) {
    return isInteger(val) || isFloating(val);
}

/**
 * @brief ポインタ型かどうかを判定
 */
inline bool isPointer(const TypedValue &val) {
    return val.type.type_info == TYPE_POINTER;
}

/**
 * @brief 参照型かどうかを判定（未実装）
 * 注: 現在の実装ではTYPE_REFERENCEは存在しないため常にfalseを返します
 */
inline bool isReference(const TypedValue &val) {
    return false; // TYPE_REFERENCEは現在未実装
}

/**
 * @brief 配列型かどうかを判定
 */
inline bool isArray(const TypedValue &val) { return val.type.is_array; }

/**
 * @brief 構造体型かどうかを判定
 */
inline bool isStruct(const TypedValue &val) {
    return val.type.type_info == TYPE_STRUCT;
}

/**
 * @brief Union型かどうかを判定
 */
inline bool isUnion(const TypedValue &val) {
    return val.type.type_info == TYPE_UNION;
}

/**
 * @brief 関数ポインタ型かどうかを判定
 */
inline bool isFunctionPointer(const TypedValue &val) {
    return val.type.type_info == TYPE_FUNCTION_POINTER;
}

/**
 * @brief 文字列型かどうかを判定
 */
inline bool isString(const TypedValue &val) {
    return val.type.type_info == TYPE_STRING;
}

/**
 * @brief ブーリアン型かどうかを判定
 */
inline bool isBoolean(const TypedValue &val) {
    return val.type.type_info == TYPE_BOOL;
}

/**
 * @brief void型かどうかを判定
 */
inline bool isVoid(const TypedValue &val) {
    return val.type.type_info == TYPE_VOID;
}

// ===========================
// TypeInfo直接チェック（Variable/ReturnException用）
// ===========================

/**
 * @brief 整数型かどうかを判定（TypeInfo版）
 */
inline bool isInteger(TypeInfo type) {
    return type == TYPE_TINY || type == TYPE_SHORT || type == TYPE_INT ||
           type == TYPE_LONG;
}

/**
 * @brief 浮動小数点型かどうかを判定（TypeInfo版）
 */
inline bool isFloating(TypeInfo type) {
    return type == TYPE_FLOAT || type == TYPE_DOUBLE;
}

/**
 * @brief 数値型かどうかを判定（TypeInfo版）
 */
inline bool isNumeric(TypeInfo type) {
    return isInteger(type) || isFloating(type);
}

/**
 * @brief ポインタ型かどうかを判定（TypeInfo版）
 */
inline bool isPointer(TypeInfo type) { return type == TYPE_POINTER; }

/**
 * @brief 構造体型かどうかを判定（TypeInfo版）
 */
inline bool isStruct(TypeInfo type) { return type == TYPE_STRUCT; }

/**
 * @brief インターフェース型かどうかを判定（TypeInfo版）
 */
inline bool isInterface(TypeInfo type) { return type == TYPE_INTERFACE; }

/**
 * @brief Union型かどうかを判定（TypeInfo版）
 */
inline bool isUnion(TypeInfo type) { return type == TYPE_UNION; }

/**
 * @brief 文字列型かどうかを判定（TypeInfo版）
 */
inline bool isString(TypeInfo type) { return type == TYPE_STRING; }

/**
 * @brief ブーリアン型かどうかを判定（TypeInfo版）
 */
inline bool isBoolean(TypeInfo type) { return type == TYPE_BOOL; }

/**
 * @brief void型かどうかを判定（TypeInfo版）
 */
inline bool isVoid(TypeInfo type) { return type == TYPE_VOID; }

// ===========================
// 複合型チェック
// ===========================

/**
 * @brief ポインタまたは参照型かどうかを判定
 */
inline bool isPointerOrReference(const TypedValue &val) {
    return isPointer(val) || isReference(val);
}

/**
 * @brief 間接参照可能な型かどうかを判定
 */
inline bool isDereferenceable(const TypedValue &val) {
    return isPointer(val) || isArray(val);
}

/**
 * @brief 集約型かどうかを判定（配列、構造体、Union）
 */
inline bool isAggregate(const TypedValue &val) {
    return isArray(val) || isStruct(val) || isUnion(val);
}

/**
 * @brief 呼び出し可能な型かどうかを判定
 */
inline bool isCallable(const TypedValue &val) { return isFunctionPointer(val); }

// ===========================
// 型名取得
// ===========================

/**
 * @brief 型名を文字列で取得
 */
inline const char *getTypeName(TypeInfo type) {
    switch (type) {
    case TYPE_TINY:
        return "tiny";
    case TYPE_SHORT:
        return "short";
    case TYPE_INT:
        return "int";
    case TYPE_LONG:
        return "long";
    case TYPE_FLOAT:
        return "float";
    case TYPE_DOUBLE:
        return "double";
    case TYPE_BOOL:
        return "bool";
    case TYPE_STRING:
        return "string";
    case TYPE_POINTER:
        return "pointer";
    case TYPE_STRUCT:
        return "struct";
    case TYPE_UNION:
        return "union";
    case TYPE_FUNCTION_POINTER:
        return "function_pointer";
    case TYPE_VOID:
        return "void";
    default:
        return "unknown";
    }
}

/**
 * @brief TypedValueの型名を取得
 */
inline const char *getTypeName(const TypedValue &val) {
    return getTypeName(val.type.type_info);
}

// ===========================
// 型の互換性チェック
// ===========================

/**
 * @brief 2つの型が同じカテゴリかどうかを判定
 */
inline bool isSameCategory(const TypedValue &a, const TypedValue &b) {
    if (isInteger(a) && isInteger(b))
        return true;
    if (isFloating(a) && isFloating(b))
        return true;
    if (isPointer(a) && isPointer(b))
        return true;
    if (isArray(a) && isArray(b))
        return true;
    if (isStruct(a) && isStruct(b))
        return true;
    return a.type.type_info == b.type.type_info;
}

/**
 * @brief 暗黙の型変換が可能かどうかを判定
 */
inline bool isImplicitlyConvertible(const TypedValue &from,
                                    const TypedValue &to) {
    // 同じ型は常に変換可能
    if (from.type.type_info == to.type.type_info)
        return true;

    // 数値型間の変換
    if (isNumeric(from) && isNumeric(to))
        return true;

    // 配列からポインタへの変換
    if (isArray(from) && isPointer(to))
        return true;

    return false;
}

// ===========================
// v0.9.2: 型変換・サイズ関連 (New)
// ===========================

/**
 * @brief 明示的なキャストが必要かどうかを判定
 * @param from 変換元の型
 * @param to 変換先の型
 * @return true if explicit cast is needed
 */
inline bool needsExplicitCast(const TypedValue &from, const TypedValue &to) {
    // 暗黙的に変換可能な場合はキャスト不要
    if (isImplicitlyConvertible(from, to))
        return false;
    
    // ポインタ型と整数型の変換は明示的キャストが必要
    if ((isPointer(from) && isInteger(to)) || 
        (isInteger(from) && isPointer(to)))
        return true;
    
    // 浮動小数点から整数への変換は明示的キャストが必要
    if (isFloating(from) && isInteger(to))
        return true;
    
    // 構造体型の変換は明示的キャストが必要
    if (isStruct(from) || isStruct(to))
        return true;
    
    return true;
}

/**
 * @brief 2つの数値型の共通型を取得
 * @param type1 型1
 * @param type2 型2
 * @return より大きい方の型（昇格規則に従う）
 */
inline TypeInfo getCommonNumericType(TypeInfo type1, TypeInfo type2) {
    // 両方とも数値型でない場合はTYPE_UNKNOWNを返す
    if (!isNumeric(type1) || !isNumeric(type2))
        return TYPE_UNKNOWN;
    
    // どちらかがdoubleならdouble
    if (type1 == TYPE_DOUBLE || type2 == TYPE_DOUBLE)
        return TYPE_DOUBLE;
    
    // どちらかがfloatならfloat
    if (type1 == TYPE_FLOAT || type2 == TYPE_FLOAT)
        return TYPE_FLOAT;
    
    // 整数型の昇格規則: long > int > short > tiny
    if (type1 == TYPE_LONG || type2 == TYPE_LONG)
        return TYPE_LONG;
    
    if (type1 == TYPE_INT || type2 == TYPE_INT)
        return TYPE_INT;
    
    if (type1 == TYPE_SHORT || type2 == TYPE_SHORT)
        return TYPE_SHORT;
    
    return TYPE_TINY;
}

/**
 * @brief 型のサイズをバイト数で取得
 * @param type 型情報
 * @return サイズ（バイト）、不明な型の場合は0
 */
inline size_t getTypeSize(TypeInfo type) {
    switch (type) {
    case TYPE_TINY:
        return 1;   // 8-bit
    case TYPE_SHORT:
        return 2;   // 16-bit
    case TYPE_INT:
        return 4;   // 32-bit
    case TYPE_LONG:
        return 8;   // 64-bit
    case TYPE_FLOAT:
        return 4;   // 32-bit
    case TYPE_DOUBLE:
        return 8;   // 64-bit
    case TYPE_BOOL:
        return 1;   // 1-byte
    case TYPE_POINTER:
        return 8;   // 64-bit pointer (assuming 64-bit system)
    case TYPE_STRING:
        return sizeof(void*); // ポインタサイズ
    case TYPE_FUNCTION_POINTER:
        return sizeof(void*); // ポインタサイズ
    default:
        return 0;   // Unknown or complex types (struct, array, etc.)
    }
}

/**
 * @brief 型のアライメント要件を取得
 * @param type 型情報
 * @return アライメント（バイト）
 */
inline size_t getTypeAlignment(TypeInfo type) {
    // Most types align to their size
    size_t size = getTypeSize(type);
    if (size == 0)
        return 8;  // Default alignment for unknown types
    
    // Alignment is typically the same as size, but max out at 8 bytes
    return size > 8 ? 8 : size;
}

/**
 * @brief 型が符号付き整数型かどうかを判定
 * @param type 型情報
 * @return true if signed integer type
 */
inline bool isSignedInteger(TypeInfo type) {
    return type == TYPE_TINY || type == TYPE_SHORT || 
           type == TYPE_INT || type == TYPE_LONG;
}

/**
 * @brief 型の最小値を取得（整数型のみ）
 * @param type 型情報
 * @param is_unsigned 符号なしフラグ
 * @return 最小値
 */
inline int64_t getTypeMinValue(TypeInfo type, bool is_unsigned) {
    if (is_unsigned)
        return 0;
    
    switch (type) {
    case TYPE_TINY:
        return -128;        // -2^7
    case TYPE_SHORT:
        return -32768;      // -2^15
    case TYPE_INT:
        return -2147483648; // -2^31
    case TYPE_LONG:
        return INT64_MIN;   // -2^63
    default:
        return 0;
    }
}

/**
 * @brief 型の最大値を取得（整数型のみ）
 * @param type 型情報
 * @param is_unsigned 符号なしフラグ
 * @return 最大値
 */
inline int64_t getTypeMaxValue(TypeInfo type, bool is_unsigned) {
    if (is_unsigned) {
        switch (type) {
        case TYPE_TINY:
            return 255;             // 2^8 - 1
        case TYPE_SHORT:
            return 65535;           // 2^16 - 1
        case TYPE_INT:
            return 4294967295;      // 2^32 - 1
        case TYPE_LONG:
            return INT64_MAX;       // Cannot represent full 2^64-1 in int64_t
        default:
            return 0;
        }
    } else {
        switch (type) {
        case TYPE_TINY:
            return 127;         // 2^7 - 1
        case TYPE_SHORT:
            return 32767;       // 2^15 - 1
        case TYPE_INT:
            return 2147483647;  // 2^31 - 1
        case TYPE_LONG:
            return INT64_MAX;   // 2^63 - 1
        default:
            return 0;
        }
    }
}

} // namespace TypeHelpers

#endif // TYPE_HELPERS_H
