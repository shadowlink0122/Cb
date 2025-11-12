# エラー伝播機能（? オペレーター）の実装設計

## 構文

```cb
expression?
```

## セマンティクス

### 1. Result<T, E>の場合

```cb
// 元のコード
let result = some_function()?;

// 展開後
let result = match some_function() {
    Ok(value) => value,
    Err(e) => return Err(e)
};
```

### 2. Option<T>の場合

```cb
// 元のコード
let value = some_optional()?;

// 展開後
let value = match some_optional() {
    Some(v) => v,
    None => return None
};
```

### 3. Future<Result<T, E>>の場合（async関数内）

```cb
// 元のコード
let value = await some_async_function()?;

// 展開後
let value = match await some_async_function() {
    Ok(v) => v,
    Err(e) => return Err(e)
};
```

## 実装ステップ

### Phase 1: パーサー拡張
- `TOK_QUESTION`は既に存在（三項演算子用）
- 後置単項演算子として`?`を認識
- `ASTNodeType::AST_ERROR_PROPAGATION`ノード追加

### Phase 2: 型チェック
- `?`を使える型を制限：
  - `Result<T, E>`
  - `Option<T>`
  - `Future<Result<T, E>>`
  - `Future<Option<T>>`
- 戻り値型が対応する型であることを検証

### Phase 3: インタープリター実装
- enum variantのチェック
- Ok/Some の場合：値を取り出す
- Err/None の場合：早期リターン

### Phase 4: 糖衣構文の展開
- ASTレベルでmatch文に展開
- または実行時に動的に処理

## 制約

1. **戻り値型の一致**：
   - `Result<T, E>?`を使う関数は`Result<T2, E>`を返す必要
   - Eのエラー型が一致する必要

2. **async文脈**：
   - `Future<Result<T, E>>`の場合、async関数内でのみ使用可能

3. **ネスト**：
   - `some_func()??` は許可（2回適用）
   - `await some_func()??` も可能

## 例

```cb
// 複数のエラーチェックを簡潔に
async Future<Result<string, string>> process_data(int id) {
    // 各ステップでエラーが発生したら即座にreturn
    int data = await fetch_data(id)?;
    int validated = validate(data)?;
    string formatted = format(validated)?;
    
    return Result<string, string>::Ok(formatted);
}

// 従来の冗長な書き方と比較
async Future<Result<string, string>> process_data_verbose(int id) {
    Future<Result<int, string>> f1 = fetch_data(id);
    Result<int, string> r1 = await f1;
    match (r1) {
        Err(e) => return Result<string, string>::Err(e),
        Ok(data) => {
            Result<int, string> r2 = validate(data);
            match (r2) {
                Err(e) => return Result<string, string>::Err(e),
                Ok(validated) => {
                    Result<string, string> r3 = format(validated);
                    match (r3) {
                        Err(e) => return Result<string, string>::Err(e),
                        Ok(formatted) => {
                            return Result<string, string>::Ok(formatted);
                        }
                    }
                }
            }
        }
    }
}
```

## 利点

1. **可読性**: エラーハンドリングのボイラープレートを削減
2. **保守性**: ハッピーパスが明確
3. **型安全性**: コンパイル時に型チェック
4. **一貫性**: Rust、Swift、Kotlinなどと同じパターン

