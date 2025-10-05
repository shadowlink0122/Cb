#pragma once

#include <cstdint>
#include <string>
#include "../../../common/ast.h"

// 前方宣言
struct Variable;

namespace PointerSystem {

// ポインタが指す対象の種類
enum class PointerTargetType {
    VARIABLE,       // 通常の変数へのポインタ
    ARRAY_ELEMENT,  // 配列要素へのポインタ
    STRUCT_MEMBER,  // 構造体メンバーへのポインタ
    NULLPTR_VALUE   // nullポインタ
};

// ポインタメタデータ構造体
// ポインタが何を指しているかの情報を保持
struct PointerMetadata {
    PointerTargetType target_type;
    
    // 真のポインタシステム用：実際のメモリアドレス
    uintptr_t address;           // 実際のアドレス（Variable*等のアドレス）
    TypeInfo pointed_type;       // 指している型
    size_t type_size;            // 型のサイズ（バイト単位）
    
    // 範囲チェック用情報（オプション）
    Variable* array_var;         // 配列の場合の配列変数
    size_t array_start_addr;     // 配列の開始アドレス
    size_t array_end_addr;       // 配列の終了アドレス
    
    // レガシー情報（後方互換性のため保持）
    Variable* var_ptr;
    size_t element_index;
    TypeInfo element_type;
    Variable* member_var;
    std::string member_path;
    
    // コンストラクタ
    PointerMetadata() 
        : target_type(PointerTargetType::NULLPTR_VALUE),
          address(0),
          pointed_type(TYPE_UNKNOWN),
          type_size(0),
          array_var(nullptr),
          array_start_addr(0),
          array_end_addr(0),
          var_ptr(nullptr),
          element_index(0),
          element_type(TYPE_UNKNOWN),
          member_var(nullptr),
          member_path("") {
    }
    
    // 静的ファクトリメソッド
    static PointerMetadata create_variable_pointer(Variable* var);
    static PointerMetadata create_array_element_pointer(Variable* array_var, size_t index, TypeInfo elem_type);
    static PointerMetadata create_struct_member_pointer(Variable* member_var, const std::string& path);
    static PointerMetadata create_nullptr();
    
    // ヘルパーメソッド
    bool is_null() const { return target_type == PointerTargetType::NULLPTR_VALUE; }
    bool is_variable() const { return target_type == PointerTargetType::VARIABLE; }
    bool is_array_element() const { return target_type == PointerTargetType::ARRAY_ELEMENT; }
    bool is_struct_member() const { return target_type == PointerTargetType::STRUCT_MEMBER; }
    
    // 値の読み取り・書き込み
    int64_t read_int_value() const;
    void write_int_value(int64_t value);
    
    double read_float_value() const;
    void write_float_value(double value);
    
    // デバッグ用
    std::string to_string() const;
};

// ポインタ値のラッパー
// 既存の変数ポインタ（int64_t）と新しいメタデータポインタを統一的に扱う
struct PointerValue {
    bool has_metadata;  // メタデータを持つか（新方式）
    
    union {
        int64_t raw_pointer;           // 従来の方式（Variable*をint64_tにキャスト）
        PointerMetadata* metadata;     // 新しい方式（メタデータ）
    };
    
    PointerValue() : has_metadata(false), raw_pointer(0) {}
    
    // 従来の方式用コンストラクタ
    explicit PointerValue(int64_t ptr) : has_metadata(false), raw_pointer(ptr) {}
    
    // 新しい方式用コンストラクタ
    explicit PointerValue(PointerMetadata* meta) : has_metadata(true), metadata(meta) {}
    
    // nullptrチェック
    bool is_null() const {
        if (has_metadata) {
            return metadata == nullptr || metadata->is_null();
        }
        return raw_pointer == 0;
    }
    
    // 変数ポインタへの変換（後方互換性）
    Variable* as_variable_pointer() const;
    
    // メタデータの取得
    PointerMetadata* get_metadata() const {
        return has_metadata ? metadata : nullptr;
    }
};

} // namespace PointerSystem
