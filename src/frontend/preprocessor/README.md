# Cb言語 プリプロセッサ

## 概要

Cb言語のプリプロセッサは、C/C++の伝統的なプリプロセッサと同様に、ソースコードを**パース前**に処理します。

## アーキテクチャ

```
ソースファイル (*.cb)
  ↓
【プリプロセッサ】 ← このモジュール
  ↓
プリプロセス済みソースコード
  ↓
Lexer → Parser → Interpreter
```

## ディレクトリ構成

```
src/frontend/preprocessor/
  ├── preprocessor.h          # メインクラス
  ├── preprocessor.cpp        # 実装
  ├── macro_definition.h      # マクロ定義の構造体
  ├── macro_expander.h        # マクロ展開器
  ├── macro_expander.cpp      # 実装
  ├── directive_parser.h      # ディレクティブパーサー
  ├── directive_parser.cpp    # 実装
  └── README.md               # このファイル
```

## サポートされるディレクティブ

### Phase 1（v0.11.0）
- `#define` - マクロ定義
- `#undef` - マクロ未定義化

### Phase 2（将来）
- `#if` / `#else` / `#endif` - 条件付きコンパイル
- `#ifdef` / `#ifndef` - マクロ定義チェック
- `#include` - ファイルインクルード

## 使用例

### 1. 単純なマクロ定義

```cb
#define PI 3.14159
#define TRUE 1
#define FALSE 0

int main() {
    println("PI =", PI);
    return 0;
}
```

### 2. 関数形式マクロ

```cb
#define SQUARE(x) ((x) * (x))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

int main() {
    int result = SQUARE(5);
    int max_val = MAX(10, 20);
    return 0;
}
```

### 3. 文字列化演算子

```cb
#define STRINGIFY(x) #x
#define DEBUG_PRINT(expr) println(#expr " = " expr)

int main() {
    int count = 42;
    DEBUG_PRINT(count);  // "count = 42"
    return 0;
}
```

### 4. プリプロセス結果の確認

```bash
# プリプロセス結果のみ出力
cb -E myfile.cb

# または
cb --preprocess myfile.cb
```

## API

### Preprocessor クラス

```cpp
#include "frontend/preprocessor/preprocessor.h"

CbPreprocessor::Preprocessor preprocessor;

// ソースコードをプリプロセス
std::string result = preprocessor.process(source_code, "myfile.cb");

// エラーチェック
if (preprocessor.hasError()) {
    std::cerr << preprocessor.getLastError() << std::endl;
}

// 定義済みマクロの一覧
auto macros = preprocessor.getDefinedMacros();
```

## 実装状況

- [x] 基本構造の設計
- [x] Preprocessorクラスの実装
- [x] MacroExpanderクラスの実装
- [x] DirectiveParserクラスの実装
- [x] main.cppへの統合
- [x] テストケースの作成（54/54パス）
- [x] 実機テスト（3つのデモプログラムで確認）

## 設計方針

1. **パース前処理**: ASTを作成する前にテキスト処理を完了
2. **独立性**: パーサーやインタープリタと疎結合
3. **デバッグ可能**: `-E`オプションで展開結果を確認可能
4. **エラー明確化**: エラー位置を正確に報告

## 参考資料

- [プリプロセッサ実装計画](../../../docs/features/preprocessor_implementation_plan.md)
- [マクロ設計見直し](../../../docs/features/macro_design_review.md)
- C/C++プリプロセッサ仕様
