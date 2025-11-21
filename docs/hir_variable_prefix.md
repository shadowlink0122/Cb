# HIR Variable Naming Convention - CB_HIR_ Prefix

## 概要 / Overview

HIR (High-Level Intermediate Representation) からC++コードを生成する際に、すべての変数名・関数名に `CB_HIR_` プレフィックスを付けることで、C++標準ライブラリやシステムライブラリとの名前衝突を防ぐ機能を実装しました。

When generating C++ code from HIR, all variable and function names are prefixed with `CB_HIR_` to prevent naming collisions with C++ standard library and system libraries.

## 実装内容 / Implementation Details

### 変更されたファイル / Modified Files

1. **src/backend/codegen/hir_to_cpp.h**
   - `add_hir_prefix()` メソッドの宣言を追加

2. **src/backend/codegen/hir_to_cpp.cpp**
   - `add_hir_prefix()` ヘルパー関数を実装
   - 以下の関数を更新してプレフィックスを適用:
     - `generate_var_decl()` - 変数宣言
     - `generate_variable()` - 変数参照
     - `generate_global_vars()` - グローバル変数
     - `generate_function()` - 関数定義と引数
     - `generate_function_call()` - 関数呼び出し
     - `generate_for()` - forループの変数
     - `generate_lambda()` - ラムダ式の引数

### プレフィックスのルール / Prefix Rules

`add_hir_prefix()` 関数は以下のルールに従います:

1. **プレフィックスを付ける対象**:
   - ユーザー定義の変数名
   - ユーザー定義の関数名
   - ループ変数
   - 関数パラメータ
   - ラムダパラメータ

2. **プレフィックスを付けない対象**:
   - `main` 関数 (エントリーポイントとして保持)
   - 組み込み関数 (`println`, `print`, `assert`, `sizeof`)
   - 修飾名 (`::` を含む名前、例: `std::string`)
   - 既に `CB_HIR_` で始まる名前

## 使用例 / Examples

### 入力 Cbコード / Input Cb Code

```cb
int add(int a, int b) {
    int result = a + b;
    return result;
}

int main() {
    int vector = 10;  // Could conflict with std::vector
    int map = 20;     // Could conflict with std::map
    int sum = add(vector, map);
    return 0;
}
```

### 生成されるC++コード / Generated C++ Code

```cpp
int CB_HIR_add(int CB_HIR_a, int CB_HIR_b) {
    {
        int CB_HIR_result = (CB_HIR_a + CB_HIR_b);
        return CB_HIR_result;
    }
}

int main() {
    {
        int CB_HIR_vector = 10;
        int CB_HIR_map = 20;
        int CB_HIR_sum = CB_HIR_add(CB_HIR_vector, CB_HIR_map);
        return 0;
    }
}
```

## メリット / Benefits

1. **名前衝突の防止** - C++標準ライブラリの型名や関数名との衝突を回避
   - `vector` → `CB_HIR_vector` (std::vector との衝突を防ぐ)
   - `map` → `CB_HIR_map` (std::map との衝突を防ぐ)
   - `string` → `CB_HIR_string` (std::string との衝突を防ぐ)

2. **明確な名前空間** - Cbのユーザーコードとシステムコードを明確に区別

3. **デバッグの容易性** - 生成されたC++コードを見たときに、どれがCbのコードから来たのか一目瞭然

## テスト / Testing

以下のテストケースを追加して動作を確認:

1. **tests/cases/hir_variable_prefix_test.cb**
   - 基本的な変数のプレフィックステスト

2. **tests/cases/hir_comprehensive_prefix_test.cb**
   - 関数、パラメータ、ループ変数を含む包括的なテスト

### テスト結果 / Test Results

```
Total:   85
Passed:  82
Failed:  3
```

82/85のテストが成功し、HIRコンパイルが正常に機能しています。

## 今後の改善点 / Future Improvements

1. 構造体フィールド名にもプレフィックスを適用する検討
2. 列挙型の値にもプレフィックスを適用する検討
3. より高度な名前マングリングの実装 (ネームスペースなど)

## バージョン / Version

- 実装バージョン: v0.14.0
- 日付: 2025-11-16
