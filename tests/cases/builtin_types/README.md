# Builtin Types (Option<T> & Result<T, E>) テストスイート

## 概要

v0.11.0で導入された組み込み型Option<T>とResult<T, E>の動作を検証するテストスイートです。

これらの型は**自動的に利用可能**で、importやenum定義は不要です。

## 基本構文

### Option<T>

```cb
// 定義不要 - 自動的に利用可能
Option<int> some_value = Option<int>::Some(42);
Option<int> none_value = Option<int>::None;

match (some_value) {
    Some(value) => {
        println("Value: ", value);
    }
    None => {
        println("No value");
    }
}
```

### Result<T, E>

```cb
// 定義不要 - 自動的に利用可能
Result<int, int> ok_value = Result<int, int>::Ok(100);
Result<int, int> err_value = Result<int, int>::Err(-1);

match (ok_value) {
    Ok(value) => {
        println("Success: ", value);
    }
    Err(error) => {
        println("Error: ", error);
    }
}
```

## テストファイル

### 1. option_basic.cb
Option<T>の基本機能テスト

**テスト内容**:
- Some(T)の作成とパターンマッチング
- Noneの作成とパターンマッチング
- 型パラメータの動作確認

**実行方法**:
```bash
./main tests/cases/builtin_types/option_basic.cb
```

**期待される出力**:
```
Some: 
42
None (expected)
Option<T> builtin test passed!
```

---

### 2. result_basic.cb
Result<T, E>の基本機能テスト

**テスト内容**:
- Ok(T)の作成とパターンマッチング
- Err(E)の作成とパターンマッチング
- 異なる型パラメータの動作確認

**実行方法**:
```bash
./main tests/cases/builtin_types/result_basic.cb
```

**期待される出力**:
```
Ok: 
100
Err: 
0
 (expected)
Result<T, E> builtin test passed!
```

---

### 3. function_return.cb
関数の戻り値としてのOption/Result使用テスト

**テスト内容**:
- Option<T>を返す関数
- Result<T, E>を返す関数
- 複雑な条件分岐での使用

**実行方法**:
```bash
./main tests/cases/builtin_types/function_return.cb
```

**期待される出力**:
```
Found: 
42
Not found (expected)
Result: 
5
Error: 
-1
 (expected)
Function return builtin test passed!
```

---

### 4. error_redefine_option.cb
Option型の重複定義エラーテスト（エラーケース）

**テスト内容**:
- 組み込み型Optionを再定義しようとするとエラーになることを確認

**実行方法**:
```bash
./main tests/cases/builtin_types/error_redefine_option.cb
```

**期待される出力**:
```
error: Cannot redefine builtin type 'Option'. Option<T> and Result<T, E> are automatically available without definition.
```

**期待される終了コード**: 1 (エラー)

---

### 5. error_redefine_result.cb
Result型の重複定義エラーテスト（エラーケース）

**テスト内容**:
- 組み込み型Resultを再定義しようとするとエラーになることを確認

**実行方法**:
```bash
./main tests/cases/builtin_types/error_redefine_result.cb
```

**期待される出力**:
```
error: Cannot redefine builtin type 'Result'. Option<T> and Result<T, E> are automatically available without definition.
```

**期待される終了コード**: 1 (エラー)

---

### 6. debug_none_match.cb
Noneバリアントのマッチングデバッグテスト

**テスト内容**:
- Option<int>::Noneの正常なマッチング動作確認

**実行方法**:
```bash
./main tests/cases/builtin_types/debug_none_match.cb
```

---

### 7. debug_function_none.cb
関数から返されるNoneのデバッグテスト

**テスト内容**:
- 関数の戻り値としてのOption<int>::Noneの動作確認

**実行方法**:
```bash
./main tests/cases/builtin_types/debug_function_none.cb
```

**既知の問題**: 関数からNoneを返した場合のマッチングに問題がある可能性（調査中）

---

## 全テスト実行

```bash
# 成功するテスト
for file in option_basic result_basic; do
    ./main tests/cases/builtin_types/${file}.cb
done

# エラーテスト（exit code != 0が期待される）
for file in error_redefine_option error_redefine_result; do
    ./main tests/cases/builtin_types/${file}.cb
    echo "Exit code: $?"
done
```

## Integration Test実行

```bash
# 全テストを実行（このテストスイートを含む）
make integration-test

# または個別に
./tests/integration/test_main
```

## 実装の詳細

### パーサー側の実装

Option<T>とResult<T, E>は、`RecursiveParser`の初期化時に自動的にenum定義として登録されます。

**実装場所**: `src/frontend/recursive_parser/recursive_parser.cpp`

```cpp
void RecursiveParser::initialize_builtin_types() {
    // Option<T> enum定義
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
    
    enum_definitions_["Option"] = option_def;
    
    // Result<T, E> enum定義
    // ... (同様の実装)
}
```

### インタプリタ側の実装

**実装場所**: `src/backend/interpreter/core/builtin_types.cpp`

```cpp
void Interpreter::initialize_builtin_types() {
    // Option<T>をEnum Managerに登録
    register_builtin_enum_option();
    
    // Result<T, E>をEnum Managerに登録
    register_builtin_enum_result();
}
```

### 重複定義エラー検出

**実装場所**: `src/frontend/recursive_parser/parsers/enum_parser.cpp`

```cpp
// v0.11.0: 組み込み型との重複チェック
if (enum_name == "Option" || enum_name == "Result") {
    parser_->error("Cannot redefine builtin type '" + enum_name + 
                  "'. Option<T> and Result<T, E> are automatically available without definition.");
    return nullptr;
}
```

## 実装ステータス

- [x] パーサー側でOption/Result自動登録
- [x] インタプリタ側でOption/Result自動登録
- [x] 重複定義エラー検出機能
- [x] 基本テストケース作成
- [x] エラーテストケース作成
- [x] Integration test作成
- [x] ドキュメント作成
- [ ] 関数戻り値でのNoneマッチング問題修正（調査中）

## 関連ドキュメント

- **設計ドキュメント**: `docs/features/builtin_types_option_result.md`
- **リリースノート**: `release_notes/v0.11.0.md`
- **stdlibドキュメント**: `stdlib/std/option.cb`, `stdlib/std/result.cb`

---

**作成日**: 2025年10月29日  
**バージョン**: v0.11.0  
**ステータス**: ✅ 実装完了（一部バグ調査中）
