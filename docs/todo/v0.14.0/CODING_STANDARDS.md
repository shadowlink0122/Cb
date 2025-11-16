# Cb言語プロジェクト コーディング規約

**最終更新**: 2025-11-14  
**バージョン**: 0.13.0

## 目次

1. [ファイル構成](#ファイル構成)
2. [インクルードガード](#インクルードガード)
3. [命名規則](#命名規則)
4. [コメント規約](#コメント規約)
5. [型とメモリ管理](#型とメモリ管理)

## ファイル構成

### ヘッダーファイル (.h)

```cpp
// ファイルの説明コメント（簡潔に）
#pragma once

#include <system_headers>  // システムヘッダー
#include "project_headers.h"  // プロジェクトヘッダー

// 前方宣言
struct ASTNode;
class Interpreter;

// 本体
class MyClass {
    // ...
};
```

### 実装ファイル (.cpp)

```cpp
#include "my_class.h"

#include <algorithm>  // 必要な標準ライブラリ
#include "other_project_headers.h"

// 実装
```

## インクルードガード

### 推奨方法: `#pragma once`

すべての新規ヘッダーファイルには `#pragma once` を使用してください。

```cpp
// my_header.h
#pragma once

class MyClass {
    // ...
};
```

**理由**:
- ✅ シンプルで読みやすい
- ✅ タイプミスのリスクなし
- ✅ すべての主要コンパイラでサポート

### 代替方法: 伝統的なガード

レガシーコードや互換性が必要な場合：

```cpp
// my_header.h
#ifndef MY_HEADER_H
#define MY_HEADER_H

class MyClass {
    // ...
};

#endif // MY_HEADER_H
```

**命名規則**:
- ファイル名を全て大文字に変換
- `.` を `_` に置換
- 末尾に `_H` を追加

例:
- `ast.h` → `AST_H`
- `recursive_parser.h` → `RECURSIVE_PARSER_H`
- `ffi_manager.h` → `FFI_MANAGER_H`

### ❌ やってはいけないこと

```cpp
// ❌ 悪い例: インクルードガードなし
class MyClass {
    // 二重インクルードでエラー！
};
```

## 命名規則

### クラス・構造体

PascalCase（各単語の先頭を大文字）

```cpp
class ExpressionEvaluator { };
struct ForeignFunctionDecl { };
struct ASTNode { };
```

### 関数・メソッド

snake_case（小文字 + アンダースコア）

```cpp
void evaluate_expression(const ASTNode& node);
int calculate_array_size(int dimensions);
Value call_foreign_function(const std::string& name);
```

### 変数

snake_case

```cpp
int array_size = 10;
std::string function_name = "sqrt";
bool is_pointer = false;
```

### メンバー変数

snake_case + 末尾のアンダースコア

```cpp
class Interpreter {
private:
    std::map<std::string, Variable> variables_;
    TypeManager type_manager_;
    bool is_running_;
};
```

### 定数

すべて大文字 + アンダースコア

```cpp
const int MAX_BUFFER_SIZE = 1024;
const double PI_VALUE = 3.14159;
constexpr int DEFAULT_ARRAY_SIZE = 100;
```

### Enum

```cpp
// Enum型: PascalCase
enum class TokenType {
    // メンバー: 大文字 + アンダースコア
    TOK_IDENTIFIER,
    TOK_NUMBER,
    TOK_STRING,
    TOK_FOREIGN,
    TOK_USE
};

// 使用時
TokenType type = TokenType::TOK_FOREIGN;
```

### 名前空間

小文字、必要に応じてアンダースコア

```cpp
namespace RecursiveParserNS {
    // ...
}

namespace FFI {
    // ...
}
```

## コメント規約

### ファイルヘッダーコメント

```cpp
// ============================================================================
// ffi_manager.h
// ============================================================================
// v0.13.0: FFI (Foreign Function Interface) Manager
//
// 外部Cライブラリの関数を呼び出すためのマネージャークラス。
// dlopen/dlsymを使用してライブラリを動的にロード。
//
// 【主な機能】:
// - ライブラリのロード・アンロード
// - 関数シンボルの解決
// - 型変換（Cb型 ⇔ C型）
// - 関数呼び出しの実行
//
// 【使用例】:
//   FFIManager manager;
//   manager.loadLibrary("m", "libm.dylib");
//   manager.registerFunction("m", "sqrt", signature);
//   Value result = manager.callFunction("m", "sqrt", args);
// ============================================================================
```

### クラス・構造体コメント

```cpp
// v0.13.0: 外部関数のパラメータ情報
struct ForeignParameter {
    std::string name;       // パラメータ名
    TypeInfo type;          // 型情報
    std::string type_name;  // 型名（"int", "double"など）
    bool is_unsigned;       // unsigned修飾子
    bool is_pointer;        // ポインタ型か
};
```

### 関数コメント

```cpp
// ライブラリをロードして関数をマップに登録
// @param module_name: モジュール名（"m", "math"など）
// @param library_path: ライブラリパス（"libm.dylib"など）
// @return: 成功時true、失敗時false
bool loadLibrary(const std::string& module_name,
                 const std::string& library_path);
```

### インラインコメント

```cpp
// コードの意図が明確でない場合のみコメント
if (type == TYPE_POINTER && depth > 0) {
    // ポインタの深さを考慮して型を解決
    resolved_type = resolvePointerType(base_type, depth);
}

// 自明なコードにはコメント不要
int size = array.size();  // ❌ 不要なコメント
```

### TODOコメント

```cpp
// TODO(v0.14.0): 構造体の受け渡しをサポート
// FIXME: メモリリークの可能性あり
// NOTE: この実装はMacOS専用
// HACK: 一時的な回避策、後でリファクタリング必要
```

## 型とメモリ管理

### スマートポインタの使用

```cpp
// unique_ptr: 単一所有権
std::unique_ptr<ASTNode> node = std::make_unique<ASTNode>(type);

// shared_ptr: 共有所有権（FFI宣言など）
std::shared_ptr<ForeignModuleDecl> decl = 
    std::make_shared<ForeignModuleDecl>();

// ❌ 生ポインタは避ける（特別な理由がない限り）
ASTNode* node = new ASTNode(type);  // メモリリークのリスク
```

### 型エイリアス

```cpp
// using を使用（C++11以降）
using StringMap = std::map<std::string, std::string>;
using NodePtr = std::unique_ptr<ASTNode>;

// ❌ typedef は古い（互換性が必要な場合を除く）
typedef std::map<std::string, std::string> StringMap;
```

### NULL vs nullptr

```cpp
// ✅ nullptr を使用（C++11以降）
ASTNode* node = nullptr;

// ❌ NULL は使わない
ASTNode* node = NULL;
```

### 型情報

```cpp
// 内部型表現: TypeInfo enum
TypeInfo type = TYPE_INT;

// 文字列型名: デバッグ・エラー表示用
std::string type_name = "int";

// 両方を保持（検証とデバッグのため）
struct ForeignParameter {
    TypeInfo type;          // 高速な型チェック
    std::string type_name;  // 分かりやすいエラーメッセージ
};
```

## エラーハンドリング

### 例外の使用

```cpp
// ✅ 適切な例外クラス
throw std::runtime_error("Failed to load library: " + library_name);
throw std::invalid_argument("Invalid type: " + type_name);

// ✅ カスタム例外（必要な場合）
class FFIException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};
```

### エラーチェック

```cpp
// ✅ 早期リターンで見通しを良く
if (!is_valid) {
    std::cerr << "Error: Invalid input" << std::endl;
    return false;
}

// 正常処理...
```

## ベストプラクティス

### 1. 単一責任の原則

各クラス・関数は1つの責任のみを持つ。

```cpp
// ✅ 良い例
class FFIManager {
    // FFI管理のみを担当
};

class TypeConverter {
    // 型変換のみを担当
};

// ❌ 悪い例
class FFIManagerAndTypeConverter {
    // 複数の責任を持つ
};
```

### 2. const の積極的使用

```cpp
// ✅ 変更しない引数はconst参照
void process(const std::string& name);

// ✅ 変更しないメソッドはconst
int size() const;

// ✅ constexpr for compile-time constants
constexpr int MAX_SIZE = 1024;
```

### 3. 明示的な型変換

```cpp
// ✅ static_cast を使用
int value = static_cast<int>(double_value);

// ❌ C-styleキャストは避ける
int value = (int)double_value;
```

### 4. Range-based for ループ

```cpp
// ✅ モダンなfor文
for (const auto& item : items) {
    process(item);
}

// ❌ 古いスタイル
for (size_t i = 0; i < items.size(); ++i) {
    process(items[i]);
}
```

## バージョン管理

### コミットメッセージ

```
[v0.13.0] Add FFI lexer support

- Add TOK_FOREIGN and TOK_USE keywords
- Update keyword map in recursive_lexer.cpp
- Add unit tests for new tokens
```

### ブランチ命名

```
feature/ffi-implementation
bugfix/memory-leak-fix
refactor/parser-cleanup
docs/update-readme
```

## 参考資料

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- プロジェクト内の既存コード

---

**質問や提案がある場合は、Issueを作成してください。**
