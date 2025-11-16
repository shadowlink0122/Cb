# HIR Debug Messages Implementation

## 実施日
2024-11-16

## 概要
HIR（High-level Intermediate Representation）生成時のデバッグメッセージを実装しました。コンパイル時に`-d`または`--debug`オプションで表示されます。

## 実装内容

### 1. HIR用デバッグメッセージIDの追加

`src/common/debug.h`に以下のメッセージIDを追加：

```cpp
// HIR (High-level Intermediate Representation) 関連
HIR_GENERATION_START,          // HIR生成開始
HIR_GENERATION_COMPLETE,       // HIR生成完了
HIR_FUNCTION_PROCESSING,       // 関数処理中
HIR_FUNCTION_ADDED,            // 関数追加
HIR_STRUCT_PROCESSING,         // 構造体処理中
HIR_STRUCT_ADDED,              // 構造体追加
HIR_ENUM_PROCESSING,           // 列挙型処理中
HIR_ENUM_ADDED,                // 列挙型追加
HIR_INTERFACE_PROCESSING,      // インターフェース処理中
HIR_INTERFACE_ADDED,           // インターフェース追加
HIR_IMPL_PROCESSING,           // 実装処理中
HIR_IMPL_ADDED,                // 実装追加
HIR_GLOBAL_VAR_PROCESSING,     // グローバル変数処理中
HIR_GLOBAL_VAR_ADDED,          // グローバル変数追加
HIR_FFI_FUNCTION_PROCESSING,   // FFI関数処理中
HIR_FFI_FUNCTION_ADDED,        // FFI関数追加
HIR_STATEMENT_PROCESSING,      // ステートメント処理中
HIR_EXPRESSION_PROCESSING,     // 式処理中
HIR_TYPE_RESOLUTION,           // 型解決
HIR_GENERIC_INSTANTIATION,     // ジェネリック具体化
```

### 2. HIRメッセージモジュールの実装

`src/common/debug/debug_hir_messages.cpp`:

```cpp
void init_hir_messages(std::vector<DebugMessageTemplate> &messages) {
    // HIR生成全般
    messages[static_cast<int>(DebugMsgId::HIR_GENERATION_START)] = {
        "[HIR] HIR generation started",
        "[HIR] HIR生成開始"};
    
    messages[static_cast<int>(DebugMsgId::HIR_GENERATION_COMPLETE)] = {
        "[HIR] HIR generation completed",
        "[HIR] HIR生成完了"};

    // 関数処理
    messages[static_cast<int>(DebugMsgId::HIR_FUNCTION_PROCESSING)] = {
        "[HIR_FUNC] Processing function: %s",
        "[HIR_FUNC] 関数処理中: %s"};
    
    messages[static_cast<int>(DebugMsgId::HIR_FUNCTION_ADDED)] = {
        "[HIR_FUNC] Function added: %s (params: %d, return: %s)",
        "[HIR_FUNC] 関数追加: %s (パラメータ: %d, 戻り値: %s)"};
    
    // ... その他のメッセージ
}
```

### 3. HIRジェネレータへの組み込み

`src/backend/ir/hir/hir_generator.cpp`:

```cpp
#include "../../../common/debug.h"

std::unique_ptr<HIRProgram>
HIRGenerator::generate(const std::vector<std::unique_ptr<ASTNode>> &ast_nodes) {
    DEBUG_PRINT(DebugMsgId::HIR_GENERATION_START);
    
    auto program = std::make_unique<HIRProgram>();

    for (const auto &node : ast_nodes) {
        // ... AST処理
    }

    DEBUG_PRINT(DebugMsgId::HIR_GENERATION_COMPLETE);

    return program;
}
```

### 4. DEBUG_PRINTマクロの追加

`src/common/debug.h`:

```cpp
// デバッグマクロ
#define DEBUG_PRINT(...) debug_msg(__VA_ARGS__)
#define ERROR_PRINT(...) error_msg(__VA_ARGS__)
```

## 使用例

### 基本的な使用

```bash
# HIRデバッグメッセージを表示してコンパイル
$ ./cb compile program.cb -d
[HIR] HIR generation started
[HIR] HIR generation completed
C++ code saved to: ./tmp/program.cpp
Compiling C++ code...
Compilation completed successfully!
Output binary: program.o
```

### 詳細な出力例

```bash
$ ./cb -c sample/algorithm/fibonacci.cb -d
[PARSE] Parsing started
[PARSE] AST generation completed
Compile mode: Generating HIR from AST...
[HIR] HIR generation started
[HIR] HIR generation completed
HIR generation successful!
  Functions: 2
  Structs: 0
  Enums: 0
  Interfaces: 0
  Impls: 0
  FFI Functions: 0
  Global Vars: 0
C++ code saved to: ./tmp/sample/algorithm/fibonacci.cpp
Compiling C++ code...
Compilation completed successfully!
Output binary: sample/algorithm/fibonacci.o
```

### 日本語デバッグモード

```bash
$ ./cb compile program.cb --debug-ja
[HIR] HIR生成開始
[HIR] HIR生成完了
C++コード保存先: ./tmp/program.cpp
C++コードコンパイル中...
コンパイル成功！
出力バイナリ: program.o
```

## メッセージ階層

### パーサ → AST → HIR の流れ

```
[PARSE] Parsing started
  ↓
[PARSE] AST generation completed
  ↓
[HIR] HIR generation started
  ↓
[HIR] HIR generation completed
  ↓
C++ code generation
```

### 将来の拡張（MIR/LIR追加時）

```
[PARSE] Parsing started
  ↓
[AST] AST processing
  ↓
[HIR] HIR generation
  ↓
[MIR] MIR generation (Middle-level IR)
  ↓
[LIR] LIR generation (Low-level IR)
  ↓
[CODEGEN] Code generation
```

## テスト結果

```bash
✅ コンパイル成功
✅ 4373/4373 integration tests passed
✅ HIRデバッグメッセージが正常に表示
✅ 既存機能に影響なし
```

## メリット

### 1. デバッグの容易さ ✅
- HIR生成の各ステップを追跡可能
- 問題の早期発見
- コンパイルパイプラインの可視化

### 2. 開発効率の向上 ✅
- HIR変換のデバッグが簡単
- 新機能追加時の動作確認
- パフォーマンス分析の基礎データ

### 3. 将来の拡張性 ✅
- MIR、LIRなど新しいIRレベルの追加が容易
- デバッグメッセージがモジュール化されている
- 一貫したメッセージフォーマット

### 4. 教育的価値 ✅
- コンパイラの動作を理解できる
- IRの役割を学習できる
- コード変換の過程を観察可能

## デバッグメッセージの例

### シンプルなプログラム

入力 (`test.cb`):
```cb
void main() {
    println("Hello, World!");
}
```

デバッグ出力:
```
[HIR] HIR generation started
[HIR] HIR generation completed
```

### 構造体とメソッドを含むプログラム

入力:
```cb
struct Point {
    int x;
    int y;
}

void main() {
    let p = Point { x: 10, y: 20 };
    println(p.x);
}
```

期待されるデバッグ出力（将来実装）:
```
[HIR] HIR generation started
[HIR_STRUCT] Processing struct: Point
[HIR_STRUCT] Struct added: Point (fields: 2)
[HIR_FUNC] Processing function: main
[HIR_FUNC] Function added: main (params: 0, return: void)
[HIR] HIR generation completed
```

## 今後の改善予定

### 短期（v0.14.x）
- [ ] より詳細な関数情報のデバッグ出力
- [ ] 構造体、列挙型、インターフェースの詳細情報
- [ ] グローバル変数の追加情報

### 中期（v0.15.x）
- [ ] MIR（Middle-level IR）のデバッグメッセージ
- [ ] 最適化パスのデバッグ出力
- [ ] 型推論のトレース

### 長期（v0.16.x+）
- [ ] LIR（Low-level IR）のデバッグメッセージ
- [ ] レジスタ割り当てのトレース
- [ ] コード生成の詳細ログ

## まとめ

HIRデバッグメッセージの実装により：

1. ✅ **可視化**: HIR生成プロセスが可視化された
2. ✅ **デバッグ**: コンパイル問題の特定が容易に
3. ✅ **拡張性**: MIR/LIR追加の基盤が整った
4. ✅ **一貫性**: インタプリタと同様のデバッグ体験

Cbコンパイラのデバッグ機能がより充実し、開発とトラブルシューティングが容易になりました！
