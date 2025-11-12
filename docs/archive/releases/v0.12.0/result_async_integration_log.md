# Result型とasync/awaitの統合実装ログ

**日付**: 2025年11月9日  
**バージョン**: v0.13.0 開発中  
**目標**: Result<T, E>とasync/awaitの完全統合

---

## 実装済み機能

### 1. ネストされたジェネリック型のパースサポート ✅

**実装内容**:
- `Future<Result<int, string>>`のようなネストされたジェネリック型をパース可能に
- `parseType()`に自動スタック管理を追加（スコープガード使用）
- `>>`トークンの自動分割機能を有効化

**変更ファイル**:
- `src/frontend/recursive_parser/parsers/type_utility_parser.cpp`
  - `parseType()`の開始時に`type_parameter_stack_`に空コンテキストをpush
  - 終了時に自動pop（スコープガード使用）
  - これにより`>>`が`> >`として分割される

- `src/frontend/recursive_parser/parsers/statement_parser.cpp`
  - 先読みロジックで`>>`を2つの`>`として処理
  - depth管理による安全な処理（depth >= 2の場合のみ-2）

**技術詳細**:
```cpp
// type_utility_parser.cpp
std::string TypeUtilityParser::parseType() {
    // スタックが空の場合のみ空コンテキストをpush
    bool should_manage_stack = parser_->type_parameter_stack_.empty();
    if (should_manage_stack) {
        parser_->type_parameter_stack_.push_back({});
    }
    
    // スコープガード: 関数終了時に自動pop
    auto stack_guard = [this, should_manage_stack]() {
        if (should_manage_stack && !parser_->type_parameter_stack_.empty()) {
            parser_->type_parameter_stack_.pop_back();
        }
    };
    std::shared_ptr<void> guard(nullptr, [&](void*){ stack_guard(); });
    
    // ... 型パース処理 ...
}
```

**テスト結果**:
```bash
# 成功
./main tests/cases/async/test_simple_generic.cb  # Future<int>
./main tests/cases/builtin_types/result_basic.cb  # Result<int, string>

# シフト演算との混同なし
Future<Result<int, string>> test(int x) {
    int y = x >> 2;  // 正しくシフト演算として認識
    ...
}
```

---

## 作成したテストケース

### 1. `test_async_result_basic.cb`
- Future<Result<int, string>>の基本動作
- 成功ケースとエラーケース
- ネストされたasync + Result

### 2. `test_async_result_propagation.cb`
- 手動エラーチェック版
- 複数のResult操作の組み合わせ
- 将来の?オペレーター実装の準備

### 3. `test_nested_generic_non_async.cb`
- non-async関数でのFuture<Result<T, E>>
- 直接メンバーアクセス

---

## 既知の問題

### 1. ジェネリックenumのインスタンス化エラー ❌

**症状**:
```
Error: Non-exhaustive match: no arm matched the enum variant ''
```

**原因**:
- `Result<int, string>`が`Result_int_string`にマングリング
- インスタンス化された型のenum variantが正しく設定されていない
- match文でvariant名が空文字列になる

**デバッグ出力**:
```
[GENERICS] Instantiated enum Result<int, string> as Result<int, string>
[GENERICS] Instantiated Future<Result_int_string> as Future<Result_int_string>
...
[INTERPRETER_SWITCH] Switch value: 0
Error: Non-exhaustive match: no arm matched the enum variant ''
```

**修正が必要な箇所**:
- ジェネリックenumのインスタンス化ロジック
- enum variantの正しい設定
- 型マングリングと実行時の型情報の一致

---

## 次の実装ステップ

### Phase 1: ジェネリックenumインスタンス化の修正 🔧

**タスク**:
1. `Result_int_string`インスタンス時にvariant情報を正しくコピー
2. match文でのvariant名解決の修正
3. ネストされたジェネリック型のメンバーアクセス修正

**関連ファイル**:
- `src/backend/interpreter/evaluator/functions/generic_instantiation.cpp`
- `src/backend/interpreter/managers/types/enums.cpp`

### Phase 2: ?オペレーターの実装 🚀

**設計**: `docs/features/error_propagation_design.md`を参照

**実装ステップ**:
1. **パーサー拡張**:
   - 後置単項演算子として`?`を認識
   - `ASTNodeType::AST_ERROR_PROPAGATION`追加
   - 三項演算子との区別

2. **型チェック**:
   - `Result<T, E>`、`Option<T>`のみ許可
   - 戻り値型の一致性検証

3. **インタープリター**:
   - enum variantのチェック
   - Ok/Some: 値を取り出す
   - Err/None: 早期リターン

**使用例**:
```cb
async Future<Result<int, string>> compute(int a, int b) {
    int div = await safe_divide(a, b)?;  // Errなら即return
    int validated = validate(div)?;       // Errなら即return
    return Result<int, string>::Ok(validated);
}
```

### Phase 3: Future<Result<T, E>>パターンの完全サポート 🎯

**目標**:
- `async Future<Result<T, E>>`関数の完全動作
- `await expr?`の組み合わせサポート
- エラーハンドリングのベストプラクティス確立

---

## シフト演算との混同について

### 問題の説明
`>>`は以下の2つの意味を持つ：
1. **右シフト演算子**: `x >> 2`
2. **ネストされたジェネリック型の閉じ**: `Future<Result<int, string>>`

### 解決方法

#### 1. 実行時パース（完全に解決済み） ✅
- `type_parameter_stack_`が空でない場合のみ`>>`を分割
- スタックベースの文脈管理により、型パース中のみ分割
- 式評価中は通常の右シフトとして扱う

#### 2. 先読みロジック（改善済み） ✅
```cpp
// depth >= 2の場合のみ >> を 2つの > として扱う
if (parser_->check(TokenType::TOK_RIGHT_SHIFT)) {
    if (depth >= 2) {
        depth -= 2;  // ネストされたジェネリクスの閉じ
    } else {
        depth--;     // 安全側: 1つの>として扱う
    }
}
```

### テスト
```cb
Future<Result<int, string>> test_func(int x) {
    int shifted = x >> 2;  // ✅ 正しくシフト演算として認識
    ...
}
```

---

## 統計情報

### コード変更
- 変更ファイル数: 2
- 追加行数: +45行（スコープガード + 先読み改善）
- テストファイル: 3個

### 技術的メリット
1. **自動スタック管理**: スコープガードによる安全なリソース管理
2. **文脈依存パース**: `>>`の意味を文脈で正しく判定
3. **既存コード互換**: 既存のテストは全て合格

---

## 参考資料

- `docs/features/error_propagation_design.md` - ?オペレーターの詳細設計
- `docs/features/async_await_v0.12.0_implementation.md` - async/awaitの実装詳細
- Rust Book - Error Handling: https://doc.rust-lang.org/book/ch09-00-error-handling.html

---

## まとめ

v0.13.0では、Cb言語に**モダンなエラーハンドリング機能**を追加します：

1. ✅ **ネストされたジェネリック型のパース** - 実装完了
2. 🔧 **ジェネリックenumの修正** - 進行中
3. 🚀 **?オペレーター** - 設計完了、実装待ち

これにより、非同期処理とエラーハンドリングを組み合わせた、
安全で読みやすいコードが書けるようになります。

```cb
// v0.13.0の理想形
async Future<Result<Data, Error>> fetch_and_process(int id) {
    let data = await fetch(id)?;
    let validated = validate(data)?;
    let processed = process(validated)?;
    return Ok(processed);
}
```
