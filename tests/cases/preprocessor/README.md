# Preprocessor Test Cases (v0.13.0)

このディレクトリには、Cbプリプロセッサ機能のテストケースが含まれています。

## テスト一覧

### 基本機能

| テストファイル | 説明 | 期待される出力 |
|--------------|------|---------------|
| `define_basic.cb` | 基本的な#define（数値演算） | `78.53975` |
| `define_number.cb` | 数値マクロ定義 | `42` |
| `multiple_defines.cb` | 複数のマクロ定義 | `15` |
| `redefine_warn.cb` | マクロの再定義 | `20` |
| `undef_macro.cb` | #undefによるマクロ削除 | (空) |

### 条件付きコンパイル

| テストファイル | 説明 | 期待される出力 |
|--------------|------|---------------|
| `ifdef_true.cb` | #ifdef（真の場合） | `Debug enabled` |
| `ifdef_false.cb` | #ifdef（偽の場合） | (空) |
| `ifndef_true.cb` | #ifndef（未定義の場合） | `Release mode` |
| `else_branch.cb` | #else分岐 | `Not in debug mode` |
| `elseif_branch.cb` | #elseif分岐 | `Production mode` |
| `nested_ifdef.cb` | ネストした#ifdef | `Feature A enabled` + `Feature B enabled` |

### 組み込みマクロ

| テストファイル | 説明 | 期待される出力 |
|--------------|------|---------------|
| `builtin_version.cb` | __VERSION__マクロ | `0.13.0` |

### 保護機能

| テストファイル | 説明 | 期待される出力 |
|--------------|------|---------------|
| `string_protection.cb` | 文字列内のマクロ保護 | `The value of PI is 3.14159` |
| `identifier_boundary.cb` | 識別子境界チェック | `42` |
| `partial_match.cb` | 部分一致防止 | `100` |
| `underscore_boundary.cb` | アンダースコア境界 | `42` |
| `comment_protection.cb` | コメント内のマクロ保護 | `42` |

## テストの実行方法

### 個別のテストケース実行

```bash
cd /path/to/Cb
./main tests/cases/preprocessor/define_basic.cb
```

### Integration Test実行

```bash
cd tests/integration
./test_main
```

プリプロセッサテストのみ実行する場合は、出力をフィルタリングします：

```bash
./test_main 2>&1 | grep -A 25 "Preprocessor Tests"
```

### 全テストスイート実行

```bash
cd tests/integration
./test_main
```

期待される結果：
```
[integration-test] ✅ PASS: Preprocessor Tests (v0.13.0) (52 tests)
```

## テスト設計の原則

### 1. 文字列リテラル保護

マクロは文字列リテラル内では展開されません：

```cb
#define PI 3.14159

void main() {
    println("The value of PI is 3.14159");  // PI は置換されない
}
```

### 2. 識別子境界チェック

マクロ名が識別子の一部である場合は置換されません：

```cb
#define MAX 999

void main() {
    int MAXIMUM = 100;  // MAXIMUM は置換されない
    int MAX_VALUE = 42; // MAX_VALUE も置換されない
}
```

### 3. アンダースコア処理

アンダースコアは識別子の一部として扱われます：

```cb
#define VALUE 999

void main() {
    int VALUE_MAX = 42;  // VALUE_MAX は置換されない
}
```

### 4. コメント保護

コメント内のマクロは展開されません：

```cb
#define VALUE 999

void main() {
    // This comment mentions VALUE but doesn't affect code
    int x = 42;
}
```

## コマンドラインオプション

### マクロ定義

```bash
./main -DDEBUG program.cb                    # DEBUG を定義
./main -DVERSION=13 program.cb               # VERSION=13 を定義
./main -DDEBUG -DVERSION=13 program.cb       # 複数定義
```

### プリプロセッサ無効化

```bash
./main --no-preprocess program.cb            # プリプロセッサを無効化
```

## テストカバレッジ

- ✅ 基本ディレクティブ: 9種類（#define, #undef, #ifdef, #ifndef, #elseif, #else, #endif, #error, #warning）
- ✅ 組み込みマクロ: 5種類（__FILE__, __LINE__, __DATE__, __TIME__, __VERSION__）
- ✅ 文字列保護: 完全サポート
- ✅ 識別子境界: 完全サポート
- ✅ ネスト: 完全サポート
- ✅ 再定義: サポート
- ✅ コメント保護: 完全サポート

**Total: 31 test cases, 95 assertions**

### 追加テスト（v0.13.1）

| テストファイル | 説明 | 期待される出力 |
|--------------|------|---------------|
| `builtin_file.cb` | __FILE__マクロ | ファイル名 |
| `builtin_line.cb` | __LINE__マクロ | `5` |
| `builtin_date_time.cb` | __DATE__/__TIME__マクロ | 日付と時刻 |
| `macro_expansion_order.cb` | マクロ展開順序 | `15` |
| `nested_expansion.cb` | ネストマクロ展開 | `100` |
| `ifdef_nested_else.cb` | ネスト#ifdef + #else | `Inner defined` |
| `multiple_elseif.cb` | 複数#elseif分岐 | `Option 2` |
| `empty_define.cb` | 空マクロ定義（フラグ） | `Flag is defined` |
| `macro_in_expression.cb` | 式内のマクロ | `75` |
| `undef_redefine.cb` | 再定義 | `20` |
| `ifdef_with_operators.cb` | 演算子を含むマクロ | `35` |
| `whitespace_handling.cb` | 空白処理 | `42` |
| `numeric_types.cb` | 数値型テスト | `3.14159 100 true` |
| `case_sensitive.cb` | 大文字小文字区別 | `10 20` |

## 既知の制限

以下の機能は現在未実装です（Phase 4で実装予定）：

- 関数マクロの引数展開
- 可変長引数マクロ（__VA_ARGS__）
- 複数行マクロ（バックスラッシュ継続）
- プリプロセッサ式評価（#if defined() など）

## 関連ドキュメント

- [FFI実装進捗レポート](../../../docs/todo/v0.13.0/ffi_implementation_progress.md)
- [モダンFFI・マクロ設計](../../../docs/todo/v0.13.0/modern_ffi_macro_design.md)
- [Integration Test Framework](../../integration/framework/integration_test_framework.hpp)
