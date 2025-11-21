# C++コード生成デバッグメッセージ実装サマリー
# C++ Code Generation Debug Messages Implementation Summary

## 実装内容 / Implementation Details

### 1. デバッグメッセージIDの追加 / Added Debug Message IDs

**ファイル**: `src/common/debug.h`

追加されたデバッグメッセージID（全98個）:
Added debug message IDs (total 98):

#### C++コード生成全般 / General C++ Code Generation (12)
- `CODEGEN_CPP_START` - C++コード生成開始
- `CODEGEN_CPP_COMPLETE` - C++コード生成完了
- `CODEGEN_CPP_PROGRAM` - プログラム全体の生成
- `CODEGEN_CPP_HEADER` - ヘッダー生成
- `CODEGEN_CPP_IMPORTS` - インポート生成
- `CODEGEN_CPP_TYPEDEFS` - typedef生成
- `CODEGEN_CPP_FORWARD_DECL` - 前方宣言生成
- `CODEGEN_CPP_GLOBAL_VAR` - グローバル変数生成
- `CODEGEN_CPP_FFI_FUNC` - FFI関数生成
- その他

#### 構造体・Enum・Union・Interface生成 / Struct, Enum, Union, Interface Generation (8)
- `CODEGEN_CPP_STRUCT_START/COMPLETE` - 構造体生成
- `CODEGEN_CPP_ENUM_START/COMPLETE` - Enum生成
- `CODEGEN_CPP_UNION_START/COMPLETE` - Union生成
- `CODEGEN_CPP_INTERFACE_START/COMPLETE` - Interface生成

#### 関数・Impl生成 / Function and Impl Generation (8)
- `CODEGEN_CPP_FUNCTION_START/COMPLETE` - 関数生成
- `CODEGEN_CPP_FUNCTION_SIGNATURE` - 関数シグネチャ
- `CODEGEN_CPP_FUNCTION_BODY` - 関数本体
- `CODEGEN_CPP_IMPL_START/COMPLETE` - Impl生成
- `CODEGEN_CPP_IMPL_METHOD` - Implメソッド

#### ステートメント生成 / Statement Generation (13)
- `CODEGEN_CPP_STMT_START` - ステートメント生成開始
- `CODEGEN_CPP_STMT_VAR_DECL` - 変数宣言
- `CODEGEN_CPP_STMT_ASSIGNMENT` - 代入文
- `CODEGEN_CPP_STMT_IF` - if文
- `CODEGEN_CPP_STMT_WHILE` - while文
- `CODEGEN_CPP_STMT_FOR` - for文
- `CODEGEN_CPP_STMT_RETURN` - return文
- `CODEGEN_CPP_STMT_BLOCK` - ブロック
- `CODEGEN_CPP_STMT_SWITCH` - switch文
- `CODEGEN_CPP_STMT_DEFER` - defer文
- `CODEGEN_CPP_STMT_DELETE` - delete文
- `CODEGEN_CPP_STMT_TRY_CATCH` - try-catch文
- `CODEGEN_CPP_STMT_ASSERT` - assert文

#### 式生成 / Expression Generation (16)
- `CODEGEN_CPP_EXPR_START` - 式生成開始
- `CODEGEN_CPP_EXPR_LITERAL` - リテラル式
- `CODEGEN_CPP_EXPR_VARIABLE` - 変数参照
- `CODEGEN_CPP_EXPR_BINARY_OP` - 二項演算
- `CODEGEN_CPP_EXPR_UNARY_OP` - 単項演算
- `CODEGEN_CPP_EXPR_FUNC_CALL` - 関数呼び出し
- `CODEGEN_CPP_EXPR_METHOD_CALL` - メソッド呼び出し
- `CODEGEN_CPP_EXPR_MEMBER_ACCESS` - メンバアクセス
- `CODEGEN_CPP_EXPR_ARRAY_ACCESS` - 配列アクセス
- `CODEGEN_CPP_EXPR_CAST` - キャスト
- `CODEGEN_CPP_EXPR_TERNARY` - 三項演算子
- `CODEGEN_CPP_EXPR_LAMBDA` - ラムダ式
- `CODEGEN_CPP_EXPR_STRUCT_LITERAL` - 構造体リテラル
- `CODEGEN_CPP_EXPR_ARRAY_LITERAL` - 配列リテラル
- `CODEGEN_CPP_EXPR_NEW` - new式
- `CODEGEN_CPP_EXPR_AWAIT` - await式

#### ポインタ操作生成 / Pointer Operations Generation (10) ★重要
- `CODEGEN_CPP_POINTER_TYPE_START` - ポインタ型生成開始
- `CODEGEN_CPP_POINTER_TYPE` - ポインタ型: T*
- `CODEGEN_CPP_POINTER_CONST` - const pointer: T* const
- `CODEGEN_CPP_POINTER_TO_CONST` - pointer to const: const T*
- `CODEGEN_CPP_POINTER_ADDRESS_OF` - アドレス演算子 &
- `CODEGEN_CPP_POINTER_DEREF` - デリファレンス *
- `CODEGEN_CPP_POINTER_ARROW` - アロー演算子 ->
- `CODEGEN_CPP_POINTER_NULL` - nullptr生成
- `CODEGEN_CPP_POINTER_CAST` - ポインタキャスト
- `CODEGEN_CPP_POINTER_ARITHMETIC` - ポインタ算術演算

#### 参照型生成 / Reference Type Generation (2)
- `CODEGEN_CPP_REFERENCE_TYPE` - 参照型 &
- `CODEGEN_CPP_RVALUE_REF_TYPE` - 右辺値参照 &&

#### 型生成詳細 / Type Generation Details (6)
- `CODEGEN_CPP_TYPE_START` - 型生成開始
- `CODEGEN_CPP_TYPE_BASIC` - 基本型
- `CODEGEN_CPP_TYPE_ARRAY` - 配列型
- `CODEGEN_CPP_TYPE_FUNCTION` - 関数型
- `CODEGEN_CPP_TYPE_GENERIC` - ジェネリック型
- `CODEGEN_CPP_TYPE_COMPLETE` - 型生成完了

#### ポインタ実装デバッグ / Pointer Implementation Debug (9) ★重要
- `POINTER_IMPL_ALLOC` - メモリ割り当て
- `POINTER_IMPL_FREE` - メモリ解放
- `POINTER_IMPL_COPY` - ポインタコピー
- `POINTER_IMPL_ASSIGN` - ポインタ代入
- `POINTER_IMPL_COMPARE` - ポインタ比較
- `POINTER_IMPL_NULL_CHECK` - NULLチェック
- `POINTER_IMPL_DEREF_CHECK` - デリファレンス前チェック
- `POINTER_IMPL_BOUNDS_CHECK` - 境界チェック
- `POINTER_IMPL_TYPE_MISMATCH` - 型不一致

### 2. デバッグメッセージテンプレートの実装 / Debug Message Templates Implementation

**新規ファイル**:
- `src/common/debug/debug_codegen_cpp_messages.h`
- `src/common/debug/debug_codegen_cpp_messages.cpp`

**特徴**:
- 英語と日本語のメッセージを両方サポート
- フォーマット文字列対応（`%s`, `%d`など）
- わかりやすい階層構造（インデント付き）

### 3. HIR to C++トランスパイラへのデバッグ統合 / Debug Integration into HIR to C++ Transpiler

**ファイル**: `src/backend/codegen/hir_to_cpp.cpp`

**追加されたデバッグポイント**:

#### プログラム生成 / Program Generation
- 開始時: 構造体数、関数数を表示
- 完了時: 生成された総行数を表示

#### 構造体生成 / Struct Generation
```cpp
debug_msg(DebugMsgId::CODEGEN_CPP_STRUCT_START, 
          struct_def.name.c_str(), 
          static_cast<int>(struct_def.fields.size()));
// ... 生成処理 ...
debug_msg(DebugMsgId::CODEGEN_CPP_STRUCT_COMPLETE, 
          struct_def.name.c_str());
```

#### 関数生成 / Function Generation
```cpp
debug_msg(DebugMsgId::CODEGEN_CPP_FUNCTION_START, 
          func.name.c_str(), 
          static_cast<int>(func.parameters.size()));
// ... 生成処理 ...
debug_msg(DebugMsgId::CODEGEN_CPP_FUNCTION_COMPLETE, 
          func.name.c_str(), 
          stmt_count);
```

#### ポインタ型生成 / Pointer Type Generation
```cpp
debug_msg(DebugMsgId::CODEGEN_CPP_POINTER_TYPE_START, 
          inner_type_name.c_str());

if (type.is_pointee_const) {
    debug_msg(DebugMsgId::CODEGEN_CPP_POINTER_TO_CONST, ...);
} else {
    debug_msg(DebugMsgId::CODEGEN_CPP_POINTER_TYPE, ...);
}

if (type.is_pointer_const) {
    debug_msg(DebugMsgId::CODEGEN_CPP_POINTER_CONST, ...);
}
```

#### アドレス演算子とデリファレンス / Address-of and Dereference
```cpp
// アドレス演算子
debug_msg(DebugMsgId::CODEGEN_CPP_POINTER_ADDRESS_OF, 
          operand_str.c_str());

// デリファレンス
debug_msg(DebugMsgId::CODEGEN_CPP_POINTER_DEREF, 
          operand_str.c_str());
```

#### アロー演算子 / Arrow Operator
```cpp
if (expr.is_arrow) {
    debug_msg(DebugMsgId::CODEGEN_CPP_POINTER_ARROW, 
              object_str.c_str(), 
              expr.member_name.c_str());
}
```

#### 変数宣言 / Variable Declaration
```cpp
debug_msg(DebugMsgId::CODEGEN_CPP_STMT_VAR_DECL, 
          generate_type(stmt.var_type).c_str(), 
          stmt.var_name.c_str());
```

### 4. ビルドシステムの更新 / Build System Update

**ファイル**: `Makefile`

追加:
```makefile
DEBUG_OBJS = \
    $(DEBUG_DIR)/debug_parser_messages.o \
    $(DEBUG_DIR)/debug_ast_messages.o \
    $(DEBUG_DIR)/debug_interpreter_messages.o \
    $(DEBUG_DIR)/debug_hir_messages.o \
    $(DEBUG_DIR)/debug_codegen_cpp_messages.o  # 新規追加
```

**デバッグメッセージ登録**: `src/common/debug_messages.cpp`
```cpp
DebugMessages::CodegenCpp::init_codegen_cpp_messages(messages);
```

### 5. テストとドキュメント / Testing and Documentation

#### テストプログラム / Test Program
**ファイル**: `tmp/test_pointer_debug.cb`

ポインタ操作のテスト:
- 基本的なポインタ（`int*`）
- 構造体ポインタ（`Point*`）
- アドレス演算子（`&`）
- デリファレンス（`*`）
- アロー演算子（`->`）

#### ドキュメント / Documentation
**ファイル**: `docs/DEBUG_MODE_GUIDE.md`

内容:
- デバッグモードの使用方法
- デバッグメッセージのカテゴリ一覧
- 実例とサンプル出力
- フィルタリング方法
- ポインタ実装の検証手順
- トラブルシューティング

## 使用方法 / Usage

### 基本的な使用 / Basic Usage

```bash
# コンパイル時にデバッグを有効化
./cb compile --debug test.cb

# 日本語のデバッグメッセージ
./cb compile --debug-ja test.cb

# C++コードも保存
./cb compile --debug test.cb -cpp output/
```

### デバッグメッセージのフィルタリング / Filtering Debug Messages

```bash
# ポインタ関連のみ
./cb compile --debug test.cb 2>&1 | grep "PTR"

# 構造体生成のみ
./cb compile --debug test.cb 2>&1 | grep "STRUCT"

# 関数生成のみ
./cb compile --debug test.cb 2>&1 | grep "FUNC"
```

## 実行例 / Execution Example

### 入力プログラム / Input Program

```cb
struct Point {
    int x;
    int y;
};

void test_pointers() {
    int x = 42;
    int* ptr = &x;
    int value = *ptr;
    
    Point p;
    p.x = 10;
    Point* pptr = &p;
    println("x:", pptr->x);
}

int main() {
    test_pointers();
    return 0;
}
```

### デバッグ出力（抜粋） / Debug Output (Excerpt)

```
[CODEGEN_CPP] === C++ Code Generation Started ===
[CODEGEN_CPP] Generating program (structs: 1, functions: 2)
[CODEGEN_CPP_STRUCT] Generating struct: Point (fields: 2)
[CODEGEN_CPP_STRUCT] ✓ Struct generated: Point
[CODEGEN_CPP_FUNC] Generating function: test_pointers (params: 0)
[CODEGEN_CPP_STMT]   Variable declaration: int x
[CODEGEN_CPP_PTR] Generating pointer type for: int
[CODEGEN_CPP_PTR]   Generated pointer type: int*
[CODEGEN_CPP_STMT]   Variable declaration: int* ptr
[CODEGEN_CPP_PTR]   Address-of: &x
[CODEGEN_CPP_PTR]   Dereference: *ptr
[CODEGEN_CPP_PTR] Generating pointer type for: Point
[CODEGEN_CPP_PTR]   Generated pointer type: Point*
[CODEGEN_CPP_PTR]   Arrow operator: pptr->x
[CODEGEN_CPP_FUNC] ✓ Function generated: test_pointers (statements: 8)
[CODEGEN_CPP] === C++ Code Generation Completed (lines: 120) ===
```

### 実行結果 / Execution Result

```
x: 10
```

## メリット / Benefits

1. **開発効率の向上** / Improved Development Efficiency
   - コンパイラの内部動作が可視化される
   - バグの特定が容易になる

2. **ポインタ実装の検証** / Pointer Implementation Verification
   - ポインタ型生成の各段階を確認できる
   - メモリ操作の正確性を検証できる

3. **学習と理解** / Learning and Understanding
   - HIRからC++への変換過程を学べる
   - コンパイラの動作原理を理解できる

4. **保守性の向上** / Improved Maintainability
   - デバッグメッセージが構造化されている
   - 多言語対応で国際的な開発が可能

## 今後の拡張 / Future Extensions

1. **追加のデバッグレベル** / Additional Debug Levels
   - `--debug-verbose`: より詳細な情報
   - `--debug-minimal`: 最小限の情報

2. **デバッグログファイル出力** / Debug Log File Output
   - `--debug-log <file>`: ログをファイルに保存

3. **パフォーマンス測定** / Performance Measurement
   - 各フェーズの処理時間を測定

4. **デバッグメッセージのカスタマイズ** / Debug Message Customization
   - ユーザーが特定のメッセージのみを有効化

## まとめ / Summary

このデバッグメッセージ実装により、Cbコンパイラの開発とデバッグが大幅に効率化されました。
特にポインタ実装に関しては、各操作の正確性を段階的に検証できるようになりました。

This debug message implementation significantly improves the development and debugging efficiency of the Cb compiler.
Especially for pointer implementation, it's now possible to verify the correctness of each operation step by step.

---

**実装日**: 2025-11-19
**バージョン**: Cb Compiler v0.14.0
**実装者**: コンパイラ開発チーム
