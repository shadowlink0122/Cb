# 包括的エラーハンドリング設計（v0.13.0+）

## 概要

すべてのエラー（セグフォ、アボート、NULL参照、実行時エラー等）をResult型で統一的に扱えるようにする。

## 設計理念

### 現状の問題点

```cb
// 現在: ランタイムエラーでプログラムが強制終了
void process(int* ptr) {
    int value = *ptr;  // ❌ ptrがNULLならセグフォで即死
}

// 現在: 配列境界チェックなし
void access(int arr[], int index) {
    int val = arr[index];  // ❌ 範囲外アクセスで未定義動作
}

// 現在: ゼロ除算で実行時エラー
void divide(int a, int b) {
    int result = a / b;  // ❌ bが0ならアボート
}
```

### 提案: すべてのエラーをResult型で

```cb
// 提案: ポインタデリファレンスが失敗する可能性を型で表現
Result<int, string> process(int* ptr) {
    // try演算子（または?演算子）で安全にデリファレンス
    int value = try *ptr;  // ptrがNULLならErr返却
    return Result<int, string>::Ok(value);
}

// 提案: 配列アクセスが失敗する可能性を型で表現
Result<int, string> access(int arr[], int index, int size) {
    int val = try arr[index];  // 範囲チェック付き
    return Result<int, string>::Ok(val);
}

// 提案: 除算が失敗する可能性を型で表現
Result<int, string> divide(int a, int b) {
    int result = try (a / b);  // ゼロ除算チェック
    return Result<int, string>::Ok(result);
}
```

---

## 実装する機能

### 1. try式によるランタイムエラーのキャッチ

#### 構文
```cb
Result<T, RuntimeError> result = try expression;
```

#### 対象となるランタイムエラー

| エラー種別 | 説明 | 例 |
|-----------|------|-----|
| **NullPointerError** | NULL参照 | `*null_ptr` |
| **IndexOutOfBoundsError** | 配列範囲外アクセス | `arr[999]` |
| **DivisionByZeroError** | ゼロ除算 | `10 / 0` |
| **StackOverflowError** | スタックオーバーフロー | 深すぎる再帰 |
| **HeapExhaustionError** | ヒープメモリ枯渇 | `malloc`失敗 |
| **TypeCastError** | 無効な型キャスト | 不正なダウンキャスト |
| **ArithmeticOverflowError** | 算術オーバーフロー | `INT_MAX + 1` |

#### 使用例

##### NULL参照の安全な処理
```cb
Result<int, RuntimeError> safe_deref(int* ptr) {
    return try *ptr;
}

void main() {
    int* null_ptr = null;
    Result<int, RuntimeError> r = safe_deref(null_ptr);
    
    match (r) {
        Ok(value) => { println("Value: {value}"); }
        Err(e) => { 
            match (e) {
                NullPointerError(msg) => { println("Null pointer: {msg}"); }
                _ => { println("Other error"); }
            }
        }
    }
}
```

##### 配列範囲チェック
```cb
Result<int, RuntimeError> safe_array_access(int arr[], int index) {
    return try arr[index];  // 内部で境界チェック
}

void main() {
    int arr[3];
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    
    Result<int, RuntimeError> r1 = safe_array_access(arr, 1);  // OK
    Result<int, RuntimeError> r2 = safe_array_access(arr, 10); // Err
    
    match (r2) {
        Ok(v) => { println("Value: {v}"); }
        Err(IndexOutOfBoundsError(msg)) => { 
            println("Index out of bounds: {msg}"); 
        }
    }
}
```

##### ゼロ除算の処理
```cb
Result<int, RuntimeError> safe_divide(int a, int b) {
    return try (a / b);
}

void main() {
    Result<int, RuntimeError> r = safe_divide(10, 0);
    
    match (r) {
        Ok(value) => { println("Result: {value}"); }
        Err(DivisionByZeroError(msg)) => { 
            println("Cannot divide by zero: {msg}"); 
        }
    }
}
```

---

### 2. RuntimeError列挙型

```cb
// ビルトイン型として提供
enum RuntimeError {
    NullPointerError(string),           // NULL参照
    IndexOutOfBoundsError(string),      // 配列範囲外
    DivisionByZeroError(string),        // ゼロ除算
    StackOverflowError(string),         // スタックオーバーフロー
    HeapExhaustionError(string),        // メモリ枯渇
    TypeCastError(string),              // 型キャストエラー
    ArithmeticOverflowError(string),    // オーバーフロー
    AssertionError(string),             // アサート失敗
    Custom(string)                      // カスタムエラー
}
```

#### 使用例
```cb
void process_errors() {
    Result<int, RuntimeError> r1 = try (*null_ptr);
    Result<int, RuntimeError> r2 = try (arr[999]);
    Result<int, RuntimeError> r3 = try (10 / 0);
    
    // 統一的なエラーハンドリング
    handle_result(r1);
    handle_result(r2);
    handle_result(r3);
}

void handle_result(Result<int, RuntimeError> r) {
    match (r) {
        Ok(v) => { println("Success: {v}"); }
        Err(e) => {
            match (e) {
                NullPointerError(msg) => { println("Null: {msg}"); }
                IndexOutOfBoundsError(msg) => { println("Bounds: {msg}"); }
                DivisionByZeroError(msg) => { println("DivByZero: {msg}"); }
                _ => { println("Other error"); }
            }
        }
    }
}
```

---

### 3. ?演算子によるエラー伝播

#### 構文
```cb
// エラーを自動的に上位に伝播
T value = expression?;
```

#### 動作
- 式の評価結果が`Ok(value)`なら値を返す
- `Err(e)`なら即座に関数から`Err(e)`を返す（早期return）

#### 使用例

##### 基本的な使用
```cb
Result<int, RuntimeError> complex_operation() {
    int val1 = (try *ptr)?;           // ptrがNULLならここでErr返却
    int val2 = (try arr[index])?;     // 範囲外ならここでErr返却
    int val3 = (try (val1 / val2))?;  // ゼロ除算ならここでErr返却
    
    return Result<int, RuntimeError>::Ok(val3);
}
```

##### async/awaitとの組み合わせ
```cb
async Future<Result<int, RuntimeError>> async_operation() {
    int val1 = (await fetch_data())?;     // awaitとエラー伝播
    int val2 = (try process(val1))?;       // tryとエラー伝播
    int val3 = (await save_result(val2))?; // 統合的なエラー処理
    
    return Result<int, RuntimeError>::Ok(val3);
}
```

---

### 4. checked演算子（オプトイン安全性）

#### 概念
デフォルトでは既存の動作を維持し、`checked`キーワードで明示的に安全性を要求。

```cb
// 通常: 既存動作（パフォーマンス優先）
int unsafe_access(int arr[], int index) {
    return arr[index];  // 範囲チェックなし（高速）
}

// checked: 安全性優先
Result<int, RuntimeError> safe_access(int arr[], int index) {
    return checked arr[index];  // 範囲チェックあり
}
```

#### checked式の例
```cb
Result<int, RuntimeError> checked_operations() {
    int arr[10];
    
    // checked配列アクセス
    int val1 = checked arr[5];  // 境界チェック付き
    
    // checked算術演算
    int val2 = checked (val1 + 1000);  // オーバーフローチェック
    
    // checkedポインタデリファレンス
    int* ptr = &val1;
    int val3 = checked *ptr;  // NULLチェック付き
    
    return Result<int, RuntimeError>::Ok(val3);
}
```

---

### 5. panic!マクロとunwrap()

#### panic!でプログラムを即座に終了
```cb
void critical_section() {
    if (critical_error) {
        panic!("Critical error occurred");  // プログラム終了
    }
}
```

#### unwrap()で意図的にエラーを無視
```cb
void guaranteed_success() {
    // 開発者が成功を保証する場合のみ使用
    int value = risky_operation().unwrap();  // Errならpanic
}
```

---

### 6. 関数シグネチャでのエラー表明

#### throws節（オプション）
```cb
// 関数がどのエラーを投げるか明示
Result<int, RuntimeError> divide(int a, int b) throws DivisionByZeroError {
    if (b == 0) {
        return Result<int, RuntimeError>::Err(
            RuntimeError::DivisionByZeroError("Cannot divide by zero")
        );
    }
    return Result<int, RuntimeError>::Ok(a / b);
}
```

---

## 実装段階

### Phase 1: 基本的なtry式（v0.13.0）
- ✅ try式の構文パース
- ✅ RuntimeError列挙型の定義
- ✅ 基本的なランタイムエラーのキャッチ
  - NullPointerError
  - DivisionByZeroError
  - IndexOutOfBoundsError

### Phase 2: ?演算子（v0.13.0）
- ✅ ?演算子の構文パース
- ✅ 自動エラー伝播の実装
- ✅ async/awaitとの統合

### Phase 3: checked演算子（v0.14.0）
- checked式の実装
- パフォーマンスとのバランス
- オプトインセーフティモデル

### Phase 4: 高度な機能（v0.14.0+）
- panic!マクロ
- unwrap()メソッド
- throws節
- カスタムエラー型

---

## 技術的実装詳細

### try式の実装

#### パーサー拡張
```cpp
// src/frontend/parser/recursive_parser.cpp
std::unique_ptr<ASTNode> RecursiveParser::parse_try_expression() {
    consume_token(TokenType::TRY);
    auto expr = parse_expression();
    
    // try式のASTノード作成
    auto try_node = std::make_unique<TryExpressionNode>();
    try_node->expression = std::move(expr);
    return try_node;
}
```

#### インタプリタ実装
```cpp
// src/backend/interpreter/evaluator/operators/try_operator.cpp
TypedValue Interpreter::evaluate_try_expression(const TryExpressionNode* node) {
    try {
        // 式を評価
        TypedValue result = evaluate_expression(node->expression.get());
        
        // 成功時: Result<T, RuntimeError>::Ok(value)を返す
        return create_ok_result(result);
    }
    catch (const NullPointerException& e) {
        // NULL参照を検出
        return create_runtime_error("NullPointerError", e.what());
    }
    catch (const IndexOutOfBoundsException& e) {
        // 範囲外アクセスを検出
        return create_runtime_error("IndexOutOfBoundsError", e.what());
    }
    catch (const DivisionByZeroException& e) {
        // ゼロ除算を検出
        return create_runtime_error("DivisionByZeroError", e.what());
    }
    // ... 他のランタイムエラー
}
```

### 安全な配列アクセス

#### 境界チェック追加
```cpp
// src/backend/interpreter/evaluator/access/array_access.cpp
TypedValue Interpreter::evaluate_array_access_checked(
    const Variable* arr, 
    int index
) {
    // 境界チェック
    if (index < 0 || index >= arr->array_size) {
        throw IndexOutOfBoundsException(
            "Index " + std::to_string(index) + 
            " out of bounds for array of size " + 
            std::to_string(arr->array_size)
        );
    }
    
    // 通常のアクセス
    return arr->array_values[index];
}
```

### 安全なポインタデリファレンス

```cpp
// src/backend/interpreter/evaluator/operators/pointer_ops.cpp
TypedValue Interpreter::evaluate_pointer_deref_checked(const Variable* ptr) {
    // NULLチェック
    if (ptr->ptr == nullptr) {
        throw NullPointerException("Attempted to dereference null pointer");
    }
    
    // 通常のデリファレンス
    return *ptr->ptr;
}
```

---

## 使用例: 総合的なエラーハンドリング

### ファイル読み込み処理
```cb
async Future<Result<string, RuntimeError>> read_file(string path) {
    // ファイルを開く（失敗する可能性）
    File* file = try open_file(path)?;
    
    // サイズを取得（失敗する可能性）
    int size = try file.size()?;
    
    // メモリ確保（失敗する可能性）
    char* buffer = try malloc(size)?;
    
    // 読み込み（失敗する可能性）
    int bytes_read = try file.read(buffer, size)?;
    
    // 成功
    string content = string(buffer);
    free(buffer);
    return Result<string, RuntimeError>::Ok(content);
}

void main() {
    Result<string, RuntimeError> r = await read_file("data.txt");
    
    match (r) {
        Ok(content) => { println("File content: {content}"); }
        Err(e) => {
            match (e) {
                NullPointerError(msg) => { println("Null: {msg}"); }
                HeapExhaustionError(msg) => { println("Out of memory: {msg}"); }
                _ => { println("Error: {e}"); }
            }
        }
    }
}
```

### データベース操作
```cb
async Future<Result<User, RuntimeError>> fetch_user(int id) {
    // DB接続（失敗する可能性）
    Connection* conn = try connect_to_db()?;
    
    // クエリ実行（失敗する可能性）
    ResultSet* rs = try conn.query("SELECT * FROM users WHERE id = ?", id)?;
    
    // データ取得（失敗する可能性）
    User user;
    user.id = try rs.get_int("id")?;
    user.name = try rs.get_string("name")?;
    
    return Result<User, RuntimeError>::Ok(user);
}
```

---

## メリット

### 1. 安全性の向上
- セグフォを事前に防ぐ
- NULL参照を型システムで管理
- 配列範囲外アクセスを検出

### 2. エラーハンドリングの統一
- すべてのエラーがResult型
- match式で統一的に処理
- 型システムによる網羅性チェック

### 3. 明示的なエラー処理
- エラーの可能性が型で表現される
- コンパイル時に処理漏れを検出
- ドキュメントとしても機能

### 4. 段階的な導入が可能
- 既存コードは変更不要
- tryやcheckedをオプトイン
- パフォーマンス影響を最小化

---

## Rust、SwiftとCbの比較

| 機能 | Rust | Swift | Cb（提案） |
|------|------|-------|----------|
| エラー型 | `Result<T, E>` | `throws`/`Result` | `Result<T, RuntimeError>` |
| エラー伝播 | `?`演算子 | `try`/`throws` | `?`演算子 |
| パニック | `panic!` | `fatalError` | `panic!` |
| NULL安全 | `Option<T>` | `Optional` | `try *ptr` |
| 範囲チェック | デフォルト有効 | デフォルト有効 | `try arr[i]` |
| 統一性 | ★★★★★ | ★★★★☆ | ★★★★★ |

---

## 結論

**すべてのエラーをResult型で統一**することで:

1. ✅ セグフォ、NULL参照などがコンパイル時に検出可能
2. ✅ async/await、通常関数で統一的なエラー処理
3. ✅ 型安全性とパフォーマンスの両立
4. ✅ 段階的に導入可能（既存コードとの互換性維持）

Cb言語は**Rust級の安全性**を持ちながら、**よりシンプルな構文**で実現します。

---

**提案日**: 2025年11月10日  
**対象バージョン**: v0.13.0以降  
**ステータス**: 設計提案
