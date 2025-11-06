# Phase 0 Week 3-4: ジェネリックenum実装設計

**期間**: 2025/11/10 - 2025/11/23  
**目標**: Rust風のジェネリックenum（特にOption<T>とResult<T,E>）の実装

---

## 概要

Week 1-2でジェネリック構造体を実装したので、同じアプローチでジェネリックenumを実装します。
特に重要なのは、値を保持するenum（Rust風のタグ付きユニオン）のサポートです。

### 目標とする構文

```cb
// Option<T> - Rust風のOption型
enum Option<T> {
    Some(T),
    None
};

// Result<T,E> - Rust風のResult型  
enum Result<T, E> {
    Ok(T),
    Err(E)
};

// 使用例
void main() {
    Option<int> opt = Option<int>::Some(42);
    
    if (opt == Option<int>::Some) {
        println("Has value: ");
        println(opt.value);  // 42
    }
    
    Result<int, string> result = Result<int, string>::Ok(100);
    
    if (result == Result<int, string>::Ok) {
        println("Success: ");
        println(result.value);  // 100
    }
}
```

---

## 実装計画

### Step 1: AST拡張 ✅ (既存のEnumDefinitionを拡張)

EnumDefinitionに以下を追加:
```cpp
struct EnumDefinition {
    std::string name;
    std::vector<EnumMember> members;
    
    // v0.11.0: ジェネリクス対応
    bool is_generic = false;
    std::vector<std::string> type_parameters;  // e.g., ["T", "E"]
    
    // 値を保持するenum（Rust風）
    bool has_associated_values = false;
    std::unordered_map<std::string, TypeInfo> member_value_types;  // member -> type
};

struct EnumMember {
    std::string name;
    int value;  // 既存: 整数値
    
    // v0.11.0: 関連値のサポート
    bool has_associated_value = false;
    TypeInfo associated_type = TYPE_UNKNOWN;
    std::string associated_type_name;  // 型パラメータ名（"T"など）
};
```

### Step 2: パーサー拡張

EnumParser::parseEnumDeclaration()を拡張:

1. **型パラメータリストの解析**
   ```cpp
   // enum Option<T> { ... }
   if (check(TokenType::TOK_LT)) {
       advance();  // '<'
       
       // 型パラメータのリスト解析
       do {
           if (!check(TokenType::TOK_IDENTIFIER)) {
               error("Expected type parameter name");
           }
           type_parameters.push_back(current_token_.value);
           advance();
       } while (match(TokenType::TOK_COMMA));
       
       consume(TokenType::TOK_GT, "Expected '>' after type parameters");
   }
   ```

2. **関連値の解析**
   ```cpp
   // Some(T) 構文の解析
   if (check(TokenType::TOK_LPAREN)) {
       advance();  // '('
       
       // 関連値の型を解析
       std::string value_type = parseType();
       member.has_associated_value = true;
       member.associated_type_name = value_type;
       
       consume(TokenType::TOK_RPAREN, "Expected ')' after associated value type");
   }
   ```

3. **型パラメータスタック管理**
   - StructParserと同様に、enum解析中は型パラメータスタックを管理
   - メンバーの関連値型が型パラメータかチェック

### Step 3: ジェネリックenumのインスタンス化

RecursiveParser::instantiateGenericEnum()を実装:

```cpp
void RecursiveParser::instantiateGenericEnum(
    const std::string& base_name,
    const std::vector<std::string>& type_arguments) {
    
    // 1. 元のジェネリックenum定義を取得
    auto it = enum_definitions_.find(base_name);
    if (it == enum_definitions_.end()) {
        error("Generic enum '" + base_name + "' not found");
        return;
    }
    
    const EnumDefinition& generic_enum = it->second;
    
    // 2. 型引数の数をチェック
    if (type_arguments.size() != generic_enum.type_parameters.size()) {
        error("Wrong number of type arguments");
        return;
    }
    
    // 3. インスタンス名を生成: Option<int> -> Option_int
    std::string instance_name = base_name;
    for (const auto& arg : type_arguments) {
        instance_name += "_" + arg;
    }
    
    // 4. 既にインスタンス化済みかチェック
    if (enum_definitions_.find(instance_name) != enum_definitions_.end()) {
        return;  // 既にインスタンス化済み
    }
    
    // 5. 型置換マップを作成: T -> int
    std::unordered_map<std::string, std::string> type_map;
    for (size_t i = 0; i < generic_enum.type_parameters.size(); ++i) {
        type_map[generic_enum.type_parameters[i]] = type_arguments[i];
    }
    
    // 6. 新しいenum定義を作成（型置換）
    EnumDefinition instantiated_enum;
    instantiated_enum.name = instance_name;
    instantiated_enum.is_generic = false;  // インスタンス化後は非ジェネリック
    instantiated_enum.has_associated_values = generic_enum.has_associated_values;
    
    // 7. メンバーをコピーして型置換
    for (const auto& member : generic_enum.members) {
        EnumMember new_member = member;
        
        if (member.has_associated_value) {
            // 型パラメータを具体的な型に置換
            if (type_map.find(member.associated_type_name) != type_map.end()) {
                std::string concrete_type = type_map[member.associated_type_name];
                new_member.associated_type = getTypeInfoFromString(concrete_type);
                new_member.associated_type_name = concrete_type;
            }
        }
        
        instantiated_enum.members.push_back(new_member);
    }
    
    // 8. インスタンス化されたenumを登録
    enum_definitions_[instance_name] = instantiated_enum;
}
```

### Step 4: 型解析での利用

TypeUtilityParser::parseType()を拡張:

```cpp
// Option<int> のような型を解析
if (check(TokenType::TOK_IDENTIFIER)) {
    std::string identifier = current_token_.value;
    advance();
    
    // ジェネリックenumかチェック
    if (check(TokenType::TOK_LT)) {
        auto enum_it = parser_->enum_definitions_.find(identifier);
        if (enum_it != parser_->enum_definitions_.end() && 
            enum_it->second.is_generic) {
            
            advance();  // '<'
            
            // 型引数のリストを解析
            std::vector<std::string> type_arguments;
            do {
                std::string arg = parseType();
                type_arguments.push_back(arg);
            } while (match(TokenType::TOK_COMMA));
            
            consume(TokenType::TOK_GT, "Expected '>'");
            
            // ジェネリックenumをインスタンス化
            parser_->instantiateGenericEnum(identifier, type_arguments);
            
            // インスタンス名を生成
            std::string instance_name = identifier;
            for (const auto& arg : type_arguments) {
                instance_name += "_" + arg;
            }
            
            parsed.base_type = instance_name;
            parsed.base_type_info = TYPE_ENUM;
            return instance_name;
        }
    }
}
```

### Step 5: 実行時サポート

Interpreterでジェネリックenumの値を扱う:

1. **enum値の構築**
   ```cpp
   // Option<int>::Some(42) の評価
   // 1. Option<int> のインスタンス化（parseType()で実行済み）
   // 2. Some メンバーの値 (42) を保存
   // 3. EnumValueとして返す
   ```

2. **値のアクセス**
   ```cpp
   // opt.value の評価
   // 1. opt が enum値かチェック
   // 2. 現在のvariant（Some/None）を確認
   // 3. associated_valueを返す
   ```

---

## テスト計画

### Test 1: Basic Generic Enum (basic_enum.cb)
```cb
enum Option<T> {
    Some(T),
    None
};

void main() {
    println("Generic enum parsed successfully!");
}
```
**期待**: パースが成功し、メッセージが表示される

### Test 2: Multiple Type Parameters (result_enum.cb)
```cb
enum Result<T, E> {
    Ok(T),
    Err(E)
};

void main() {
    println("Result<T,E> parsed successfully!");
}
```
**期待**: 複数型パラメータのenumがパースされる

### Test 3: Enum Instantiation (enum_instantiation.cb)
```cb
enum Option<T> {
    Some(T),
    None
};

void main() {
    Option<int> opt1;
    Option<string> opt2;
    
    println("Enum instantiation successful!");
}
```
**期待**: Option<int>とOption<string>が別々にインスタンス化される

### Test 4: Enum Value Construction (enum_value.cb)
```cb
enum Option<T> {
    Some(T),
    None
};

void main() {
    Option<int> opt = Option<int>::Some(42);
    
    println("Enum value construction successful!");
}
```
**期待**: enum値の構築が成功する

### Test 5: Enum Value Access (enum_access.cb)
```cb
enum Option<T> {
    Some(T),
    None
};

void main() {
    Option<int> opt = Option<int>::Some(42);
    
    if (opt == Option<int>::Some) {
        println("Has value: ");
        println(opt.value);  // 42
    }
    
    assert(opt.value == 42);
    println("Enum value access successful!");
}
```
**期待**: enum値のアクセスが成功し、42が表示される

### Test 6: Result Type Usage (result_usage.cb)
```cb
enum Result<T, E> {
    Ok(T),
    Err(E)
};

void main() {
    Result<int, string> result = Result<int, string>::Ok(100);
    
    if (result == Result<int, string>::Ok) {
        println("Success: ");
        println(result.value);
        assert(result.value == 100);
    }
    
    Result<int, string> error = Result<int, string>::Err("Error message");
    
    if (error == Result<int, string>::Err) {
        println("Error: ");
        println(error.value);
    }
    
    println("Result type usage successful!");
}
```
**期待**: Result型の使用が成功する

---

## 実装の優先順位

### Week 3 (2025/11/10 - 2025/11/16)
1. ✅ AST拡張（EnumDefinitionの拡張）
2. ✅ パーサー拡張（型パラメータリストの解析）
3. ✅ 関連値の解析（Some(T)構文）
4. ✅ 基本的なテスト（Test 1-3）

### Week 4 (2025/11/17 - 2025/11/23)
1. ✅ instantiateGenericEnum()の実装
2. ✅ TypeUtilityParserの拡張
3. ✅ Interpreterの実行時サポート
4. ✅ 完全なテスト（Test 4-6）
5. ✅ ドキュメント更新

---

## 成功基準

- [ ] enum Option<T> がパースできる
- [ ] enum Result<T,E> がパースできる
- [ ] Option<int>, Option<string> が別々にインスタンス化される
- [ ] Option<int>::Some(42) が評価できる
- [ ] opt.value でenum値にアクセスできる
- [ ] Result<int, string>::Ok(100) が動作する
- [ ] すべてのテストが成功する

---

## 注意点

### 既存のenum実装との互換性

Cbには既に整数値を持つenumがあります:
```cb
enum Color {
    Red = 0,
    Green = 1,
    Blue = 2
};
```

ジェネリックenumは**関連値を持つenum**として、既存のenumと区別します:
- 既存のenum: 整数値のみ（has_associated_values = false）
- ジェネリックenum: 任意の型の値を保持（has_associated_values = true）

### 実装の課題

1. **enum値の表現**
   - Option<int>::Some(42) は実行時にどう表現するか？
   - EnumValue構造体に値を保存する必要がある

2. **パターンマッチング（将来の拡張）**
   - Week 3-4では基本的なif文での比較のみ実装
   - switchでのパターンマッチングは将来のフェーズで実装

3. **メモリ管理**
   - 関連値のメモリ管理（特に動的サイズの型）
   - 現在はスタック上の値のみサポート

---

## 次のステップ

Week 3-4が完了したら:
1. Phase 0の完了報告（Week 1-4のまとめ）
2. Phase 1の計画（ジェネリック関数、トレイト、型制約など）
3. async/await実装に向けた準備（Promise<T>, Future<T>型の設計）
