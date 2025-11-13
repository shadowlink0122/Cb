# ドキュメント構文修正レポート

**日時**: 2025-11-14  
**対象**: v0.13.0 FFI関連ドキュメント  
**修正内容**: Rust風構文 → Cb正式構文

## 修正概要

v0.13.0で作成されたドキュメント内で、設計検討時に使用されたRust風の構文を、
実装されたCb言語の正式な構文に統一しました。

## 修正対象ファイル

### 1. `inline_asm_cpp_feasibility.md`

**修正箇所**: 3箇所

#### 修正前（Rust風）
```cb
use foreign "libmath.so" {
    fn add(int a, int b) -> int;
    fn sqrt(double x) -> double;
}

use lib "libmath.so" {
    add: (int, int) -> int;
    sqrt: (double) -> double;
}
```

#### 修正後（Cb正式構文）
```cb
use foreign.math {
    int add(int a, int b);
    double sqrt(double x);
}
```

**特徴**:
- ✅ `fn` キーワード削除
- ✅ `->` 演算子削除
- ✅ C言語風の関数宣言形式
- ✅ モジュール名はドット記法（`foreign.math`）

---

### 2. `modern_ffi_macro_design.md`

**修正箇所**: 10箇所以上

#### パターン1: 型情報の記法

**修正前**:
```cb
use lib "libmath.so" {
    add: (int, int) -> int;
    sqrt: (double) -> double;
}
```

**修正後**:
```cb
use foreign.math {
    int add(int a, int b);
    double sqrt(double x);
}
```

#### パターン2: ライブラリパス指定

**修正前**:
```cb
use lib "libgraphics.so" {
    draw_point: (Point) -> void;
}
```

**修正後**:
```cb
use foreign.graphics {
    void draw_point(Point p);
}
```

#### パターン3: プリプロセッサとの組み合わせ

**修正前**:
```cb
#ifdef FEATURE_NETWORKING
    use lib "libcurl.so" {
        http_get: (string) -> string;
    }
#endif
```

**修正後**:
```cb
#ifdef FEATURE_NETWORKING
    use foreign.curl {
        string http_get(string url);
    }
#endif
```

---

### 3. `ffi_implementation_progress.md`

**修正箇所**: 2箇所

#### 修正前
```cb
use lib "libmath.so" {
    add: (int, int) -> int;
    sqrt: (double) -> double;
}
```

#### 修正後
```cb
use foreign.m {
    int add(int a, int b);
    double sqrt(double x);
}
```

---

## Cb言語の正式FFI構文

### 基本形式

```cb
use foreign.module_name {
    return_type function_name(param_type param_name, ...);
    // 複数の関数を宣言可能
}
```

### 構文要素

| 要素 | 説明 | 例 |
|------|------|-----|
| `use` | インポートキーワード | `use` |
| `foreign` | FFI専用キーワード | `foreign` |
| `.module_name` | モジュール名（ドット記法） | `.math`, `.c`, `.graphics` |
| `return_type` | 戻り値の型 | `int`, `double`, `void`, `void*` |
| `function_name` | 関数名 | `add`, `sqrt`, `malloc` |
| `(...)` | パラメータリスト | `(int a, int b)` |
| `;` | 宣言の終端 | `;` |

### 特徴

1. **C言語との互換性**
   - C/C++の関数宣言と同じ形式
   - 学習コストゼロ

2. **型安全性**
   - 完全な型情報が必須
   - パラメータ名も記述（ドキュメント効果）

3. **モジュール管理**
   - モジュール名がデフォルトの名前空間
   - `math.add(10, 20)` のように呼び出し

4. **一貫性**
   - Cbの通常の関数宣言と同じ形式
   - `import`文と同じ`use`キーワード

---

## 修正理由

### 1. 設計段階の構文が残っていた

設計時に複数の構文案を検討しましたが、実装では以下の理由で
C言語風の構文を採用しました：

**採用理由**:
- ✅ C/C++との自然な統合
- ✅ 既存のCb関数宣言との一貫性
- ✅ 直感的で分かりやすい
- ✅ パラメータ名によるドキュメント効果

**不採用構文**:
- ❌ Rust風: `fn name(...) -> type`
- ❌ TypeScript風: `name: (...) => type`
- ❌ 矢印記法: `(...) -> type`

### 2. 統一性の確保

ドキュメント内で複数の構文が混在していたため、
実装された正式構文に統一しました。

### 3. ユーザーの混乱防止

学習者が間違った構文を覚えないよう、
すべてのドキュメントを正式構文に更新しました。

---

## 例外: 他言語の例示

以下のコードは**そのまま**（修正不要）：

### Rust側のコード例

```rust
// Rustのコード例（修正不要）
#[no_mangle]
pub extern "C" fn add(a: i32, b: i32) -> i32 {
    a + b
}
```

**理由**: Rust言語の正式な構文であり、Cb言語との対比として有用

### Zig側のコード例

```zig
// Zigのコード例（修正不要）
export fn add(a: i32, b: i32) i32 {
    return a + b;
}
```

**理由**: Zig言語の正式な構文であり、Cb言語との対比として有用

---

## 実装状況との整合性

### ✅ 実装済み（v0.13.0 Phase 2）

| 機能 | ステータス |
|------|-----------|
| レキサー（`foreign`, `use`） | ✅ 完了 |
| AST構造体 | ✅ 完了 |
| パーサー | ✅ 完了 |
| 構文テスト | ✅ 動作確認済み |

**実装された構文**:
```cb
use foreign.m {
    double sqrt(double x);
    double pow(double x, double y);
}
```

**テストファイル**: `tests/cases/ffi/test_ffi_parse.cb`

---

## 今後の注意事項

### ドキュメント作成時

1. **FFI構文は必ずCb正式構文を使用**
   ```cb
   use foreign.module {
       return_type function_name(params);
   }
   ```

2. **他言語との比較時は明示**
   ```markdown
   **Rust側**:
   \`\`\`rust
   pub extern "C" fn add(...) -> i32 { ... }
   \`\`\`
   
   **Cb側**:
   \`\`\`cb
   use foreign.mylib {
       int add(int a, int b);
   }
   \`\`\`
   ```

3. **非推奨構文は削除またはコメントアウト**
   ```cb
   // ❌ この構文はサポートしません
   // use "lib.so"::add(int, int) -> int;
   
   // ✅ 正しい構文
   use foreign.mylib {
       int add(int a, int b);
   }
   ```

---

## 修正完了の確認

### 検証コマンド

```bash
cd docs/todo/v0.13.0
grep -n "use lib \|: (.*) ->" *.md | grep -v "Rust\|Zig\|//"
# 結果: 0件（すべて修正済み）
```

### 修正済みファイル一覧

- ✅ `inline_asm_cpp_feasibility.md` (3箇所修正)
- ✅ `modern_ffi_macro_design.md` (10箇所以上修正)
- ✅ `ffi_implementation_progress.md` (2箇所修正)
- ✅ 他のドキュメントは元々正しい構文

---

## まとめ

### 修正内容

| 項目 | 修正前 | 修正後 |
|------|--------|--------|
| 関数宣言 | `fn name(...) -> type` | `type name(...)` |
| 型情報 | `name: (types) -> type` | `type name(params)` |
| ライブラリ | `use lib "path"` | `use foreign.module` |
| キーワード | `fn`, `->` | なし（C風） |

### 効果

1. ✅ **一貫性**: 全ドキュメントで統一された構文
2. ✅ **正確性**: 実装と完全に一致
3. ✅ **分かりやすさ**: C/C++開発者に親しみやすい
4. ✅ **学習性**: 間違った構文を学ぶリスク排除

### 検証結果

```bash
✅ 全ての不正な構文を修正
✅ 実装された構文との整合性確認
✅ テストコードで動作確認
✅ ドキュメントの一貫性確保
```

---

**修正完了日**: 2025-11-14  
**修正者**: Cb Language Development Team  
**レビュー**: 完了
