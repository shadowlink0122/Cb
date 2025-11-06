# Option/Result型の組み込み型実装

**バージョン**: v0.11.0 Complete  
**優先度**: 最高  
**ステータス**: 設計中

---

## 概要

Option<T>とResult<T, E>型を標準ライブラリ依存ではなく、**組み込み型（Built-in Types）**として実装します。これにより、インタプリタ起動時に自動的に利用可能になり、import不要で使用できます。

---

## 🎯 目標

### ✅ 達成すべきこと
1. **自動登録**: インタプリタ初期化時にOption/Resultの型定義を自動登録
2. **import不要**: どのCbプログラムからも直接使用可能
3. **パフォーマンス**: 組み込み型として最適化された実装
4. **型安全性**: ジェネリクスと完全統合

---

## 📝 設計方針

### 1. 組み込み型の定義場所

```cpp
// src/backend/interpreter/core/interpreter.cpp

void Interpreter::initialize_builtin_types() {
    // Option<T>の登録
    register_builtin_enum_option();
    
    // Result<T, E>の登録
    register_builtin_enum_result();
}

void Interpreter::register_builtin_enum_option() {
    // Option<T> enum定義を内部的に作成
    // enum Option<T> {
    //     Some(T),
    //     None
    // };
    
    EnumDefinition opt_def;
    opt_def.name = "Option";
    opt_def.is_generic = true;
    opt_def.type_parameters = {"T"};
    
    // Some(T) variant
    EnumVariant some_variant;
    some_variant.name = "Some";
    some_variant.has_associated_value = true;
    some_variant.associated_type = "T"; // 型パラメータ
    opt_def.variants.push_back(some_variant);
    
    // None variant
    EnumVariant none_variant;
    none_variant.name = "None";
    none_variant.has_associated_value = false;
    opt_def.variants.push_back(none_variant);
    
    // 内部enum定義として登録
    builtin_enum_definitions_["Option"] = opt_def;
}

void Interpreter::register_builtin_enum_result() {
    // Result<T, E> enum定義を内部的に作成
    // enum Result<T, E> {
    //     Ok(T),
    //     Err(E)
    // };
    
    EnumDefinition res_def;
    res_def.name = "Result";
    res_def.is_generic = true;
    res_def.type_parameters = {"T", "E"};
    
    // Ok(T) variant
    EnumVariant ok_variant;
    ok_variant.name = "Ok";
    ok_variant.has_associated_value = true;
    ok_variant.associated_type = "T";
    res_def.variants.push_back(ok_variant);
    
    // Err(E) variant
    EnumVariant err_variant;
    err_variant.name = "Err";
    err_variant.has_associated_value = true;
    err_variant.associated_type = "E";
    res_def.variants.push_back(err_variant);
    
    // 内部enum定義として登録
    builtin_enum_definitions_["Result"] = res_def;
}
```

### 2. 実装戦略

#### Phase 1: 組み込みEnum定義システム
```cpp
// src/backend/interpreter/core/interpreter.h

class Interpreter {
private:
    // 組み込みenum定義
    std::unordered_map<std::string, EnumDefinition> builtin_enum_definitions_;
    
    // 初期化時に呼ばれる
    void initialize_builtin_types();
    
    // 組み込み型の登録
    void register_builtin_enum_option();
    void register_builtin_enum_result();
    
public:
    // 既存のenum検索を拡張
    bool is_builtin_enum(const std::string& name) const;
    EnumDefinition get_builtin_enum(const std::string& name) const;
};
```

#### Phase 2: Enum解決の拡張
```cpp
// src/backend/interpreter/managers/enums/operations.cpp

EnumDefinition* Interpreter::find_enum_definition(const std::string& name) {
    // 1. ユーザー定義enumを検索
    auto it = enum_definitions_.find(name);
    if (it != enum_definitions_.end()) {
        return &it->second;
    }
    
    // 2. 組み込みenumを検索
    auto builtin_it = builtin_enum_definitions_.find(name);
    if (builtin_it != builtin_enum_definitions_.end()) {
        return const_cast<EnumDefinition*>(&builtin_it->second);
    }
    
    return nullptr;
}
```

#### Phase 3: ジェネリックインスタンス化
```cpp
// Option<int>, Result<int, string> などのインスタンス化
// 既存のジェネリクス実体化ロジックを使用
// builtin_enum_definitions_から型定義を取得して実体化
```

---

## 🔧 実装詳細

### ファイル構造

```
src/backend/interpreter/
├── core/
│   ├── interpreter.h           # builtin_enum_definitions_追加
│   ├── interpreter.cpp         # initialize_builtin_types()実装
│   └── builtin_types.cpp       # 新規: 組み込み型の定義
├── managers/
│   └── enums/
│       └── operations.cpp      # find_enum_definition()拡張
```

### 新規ファイル: builtin_types.cpp

```cpp
// src/backend/interpreter/core/builtin_types.cpp

#include "interpreter.h"

void Interpreter::initialize_builtin_types() {
    debug_msg(DebugMsgId::BUILTIN_TYPES_INIT);
    
    register_builtin_enum_option();
    register_builtin_enum_result();
    
    debug_msg(DebugMsgId::BUILTIN_TYPES_COMPLETE);
}

void Interpreter::register_builtin_enum_option() {
    EnumDefinition opt_def;
    opt_def.name = "Option";
    opt_def.is_generic = true;
    opt_def.type_parameters.push_back("T");
    opt_def.is_builtin = true;  // 組み込みフラグ
    
    // Some(T) variant
    EnumVariant some_var;
    some_var.name = "Some";
    some_var.has_associated_value = true;
    some_var.associated_type_name = "T";
    opt_def.variants.push_back(some_var);
    
    // None variant
    EnumVariant none_var;
    none_var.name = "None";
    none_var.has_associated_value = false;
    opt_def.variants.push_back(none_var);
    
    builtin_enum_definitions_["Option"] = opt_def;
    
    debug_msg(DebugMsgId::BUILTIN_ENUM_REGISTERED, "Option<T>");
}

void Interpreter::register_builtin_enum_result() {
    EnumDefinition res_def;
    res_def.name = "Result";
    res_def.is_generic = true;
    res_def.type_parameters.push_back("T");
    res_def.type_parameters.push_back("E");
    res_def.is_builtin = true;
    
    // Ok(T) variant
    EnumVariant ok_var;
    ok_var.name = "Ok";
    ok_var.has_associated_value = true;
    ok_var.associated_type_name = "T";
    res_def.variants.push_back(ok_var);
    
    // Err(E) variant
    EnumVariant err_var;
    err_var.name = "Err";
    err_var.has_associated_value = true;
    err_var.associated_type_name = "E";
    res_def.variants.push_back(err_var);
    
    builtin_enum_definitions_["Result"] = res_def;
    
    debug_msg(DebugMsgId::BUILTIN_ENUM_REGISTERED, "Result<T, E>");
}
```

---

## 🧪 使用例

### インタプリタ起動後すぐに使用可能

```cb
// import不要！
void main() {
    // Option<T>が自動で利用可能
    Option<int> some_val = Option<int>::Some(42);
    Option<int> none_val = Option<int>::None;
    
    // Result<T, E>も自動で利用可能
    Result<int, string> ok_val = Result<int, string>::Ok(100);
    Result<int, string> err_val = Result<int, string>::Err("error");
    
    // パターンマッチング
    match (some_val) {
        Some(value) => println("Value: ", value),
        None => println("No value"),
    }
}
```

### 関数戻り値として

```cb
Result<int, string> divide(int a, int b) {
    if (b == 0) {
        return Result<int, string>::Err("Division by zero");
    }
    return Result<int, string>::Ok(a / b);
}

void main() {
    Result<int, string> result = divide(10, 2);
    
    match (result) {
        Ok(value) => println("Result: ", value),
        Err(error) => println("Error: ", error),
    }
}
```

---

## 📊 実装スケジュール

### Phase 1: 基盤実装（1日）
- [x] EnumDefinition構造体にis_builtinフラグ追加
- [ ] builtin_enum_definitions_マップ追加
- [ ] initialize_builtin_types()実装
- [ ] builtin_types.cpp作成

### Phase 2: 登録ロジック（1日）
- [ ] register_builtin_enum_option()実装
- [ ] register_builtin_enum_result()実装
- [ ] find_enum_definition()拡張

### Phase 3: テスト（半日）
- [ ] Option<int>の基本テスト
- [ ] Result<int, string>の基本テスト
- [ ] ジェネリックインスタンス化テスト
- [ ] パターンマッチングテスト

### Phase 4: 統合（半日）
- [ ] 既存のenum検索ロジックとの統合
- [ ] デバッグメッセージ追加
- [ ] ドキュメント更新

**総見積もり**: 3日

---

## 🎯 成功基準

1. **自動登録**: インタプリタ起動時にOption/Resultが利用可能
2. **import不要**: どのCbファイルでも即座に使用可能
3. **完全互換**: 既存のenum機能と100%互換
4. **パフォーマンス**: オーバーヘッドなし
5. **テスト**: 全テスト成功（既存 + 新規）

---

## 🚀 次のステップ

1. **Phase 1実装**: builtin_types.cppの作成
2. **Phase 2実装**: 登録ロジックの実装
3. **Phase 3テスト**: 包括的なテスト
4. **Phase 4統合**: 既存コードとの統合

この実装により、Option/Result型がCb言語の**一級市民（First-class citizen）**となり、非同期処理やエラーハンドリングの基盤が完成します。

---

## 📝 備考

### 利点
- ✅ ライブラリ依存なし
- ✅ import不要
- ✅ パフォーマンス最適化
- ✅ 型安全性
- ✅ 組み込み言語機能として扱える

### 技術的課題
- ジェネリックインスタンス化の統合
- デバッグメッセージの整備
- エラーハンドリングの統一

### 将来の拡張
- 他の組み込み型の追加（Vec<T>, HashMap<K, V>など）
- 組み込みメソッドの実装（unwrap(), map()など）

---

**作成日**: 2025年10月29日  
**作成者**: v0.11.0実装チーム
