# 文字列リテラルimport構文の廃止

## 実装日
2025年11月3日

## 概要
`import "path/to/file.cb";` 形式の文字列リテラルimport構文を廃止し、`import module.path.name;` 形式のモジュールパス構文に統一しました。

## 背景

### 問題点
1. **冗長性**: `.cb`拡張子を毎回書く必要がある
2. **一貫性の欠如**: 2つの異なる構文が混在
3. **モダン言語との乖離**: Python, Rust, TypeScriptなどはモジュールパス形式

### 廃止された構文
```cb
// ❌ 廃止: 文字列リテラル形式
import "stdlib/collections/vector.cb";
import "module_a.cb";
import "../utils/helper.cb";
```

### 新しい構文
```cb
// ✅ モジュールパス形式
import stdlib.std.vector;
import module_a;
import utils.helper;
```

## 実装詳細

### パーサーの変更

#### statement_parser.cpp
```cpp
// 旧実装
ASTNode *StatementParser::parseImportStatement() {
    Token import_token = parser_->advance();
    
    std::string module_path;
    
    // 文字列リテラルをサポート
    if (parser_->check(TokenType::TOK_STRING)) {
        module_path = parser_->current_token_.value;
        parser_->advance();
    } else if (parser_->check(TokenType::TOK_IDENTIFIER)) {
        // モジュールパス形式
        // ...
    }
}

// 新実装
ASTNode *StatementParser::parseImportStatement() {
    Token import_token = parser_->advance();
    
    std::string module_path;
    
    // モジュールパス形式のみサポート
    if (parser_->check(TokenType::TOK_IDENTIFIER)) {
        module_path = parser_->current_token_.value;
        parser_->advance();
        
        // ドット記法で続くパスを結合
        while (parser_->check(TokenType::TOK_DOT)) {
            parser_->advance();
            if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
                parser_->error("Expected identifier after '.' in import path");
                delete import_node;
                return nullptr;
            }
            module_path += ".";
            module_path += parser_->current_token_.value;
            parser_->advance();
        }
    } else {
        parser_->error("Expected module path after 'import'");
        delete import_node;
        return nullptr;
    }
}
```

#### recursive_parser.cpp
```cpp
// resolveModulePath()の更新
std::string RecursiveParser::resolveModulePath(const std::string &module_path) {
    std::string file_path = module_path;

    // ドット記法の場合、パスに変換して .cb 拡張子を追加
    if (module_path.find('.') != std::string::npos &&
        module_path.find('/') == std::string::npos &&
        module_path.find("..") == std::string::npos) {
        // stdlib.collections.vector -> stdlib/collections/vector.cb
        std::replace(file_path.begin(), file_path.end(), '.', '/');
        file_path += ".cb";
    } else {
        // ドットがない場合（相対パスimport: module_name）
        // module_name -> module_name.cb
        file_path += ".cb";
    }
    
    // 検索パスでファイルを探す
    // ...
}
```

## 変更されたファイル

### コア実装
1. `src/frontend/recursive_parser/parsers/statement_parser.cpp`
   - `parseImportStatement()`: TOK_STRINGチェックを削除
   - エラーメッセージ更新

2. `src/frontend/recursive_parser/recursive_parser.cpp`
   - `resolveModulePath()`: コメント更新
   - 文字列リテラル処理の削除

### テストファイル（41ファイル）

#### stdlib内（1ファイル）
- `stdlib/async/task_queue.cb`

#### tests/cases/stdlib（26ファイル）
- allocators: 2ファイル
- collections: 20ファイル
- memory: 4ファイル

#### tests/cases/import_export（4ファイル）
- `test_basic_import_export.cb`
- `test_duplicate_import.cb`
- `test_module_helper.cb`
- `test_multiple_modules.cb`

#### tests/cases/memory（10ファイル）
- 各memcpyテストファイル

### ドキュメント（4ファイル）
- `README.md`
- `tests/cases/stdlib/README.md`
- `tests/cases/stdlib/allocators/README.md`
- `tests/cases/stdlib/collections/README.md`

## 変換例

### stdlib import
```cb
// 旧
import "stdlib/collections/vector.cb";
import "stdlib/collections/queue.cb";

// 新
import stdlib.std.vector;
import stdlib.std.queue;
```

### 相対パスimport
```cb
// 旧
import "module_a.cb";
import "string_module.cb";

// 新
import module_a;
import string_module;
```

### stdlib/memory
```cb
// 旧
import "stdlib/std/memory.cb";

// 新
import stdlib.std.memory;
```

## エラー検出

### 文字列リテラルを使用した場合
```cb
import "stdlib/collections/vector.cb";
```

**エラーメッセージ**:
```
<file>:1:10: error: Expected module path after 'import'
   1 | import "stdlib/collections/vector.cb";
     |          ^
```

### 正しい構文
```cb
import stdlib.std.vector;
```

## テスト結果

### 変換後の動作確認
```bash
# 文字列リテラル形式でエラー
$ echo 'import "stdlib/collections/vector.cb";
void main() { println("Test"); }' > /tmp/test_string_import.cb
$ ./main /tmp/test_string_import.cb
/tmp/test_string_import.cb:1:10: error: Expected module path after 'import'

# モジュールパス形式で成功
$ echo 'import stdlib.std.vector;
void main() {
    Vector<int> vec;
    vec.push_back(42);
    println("Vector length: ", vec.get_length());
}' > /tmp/test_module_import.cb
$ ./main /tmp/test_module_import.cb
Vector length:  1
```

### テスト結果
- ✅ stdlib-test: 27/27成功
- ✅ import_export tests: 正常動作
- ✅ 文字列リテラル形式: パースエラー
- ✅ モジュールパス形式: 正常動作

## 利点

### 1. 統一された構文
- 1つの明確な方法のみ
- 学習コストの低減

### 2. クリーンなコード
```cb
// 簡潔
import stdlib.std.vector;

// 冗長（廃止）
import "stdlib/collections/vector.cb";
```

### 3. モダン言語との一貫性
```python
# Python
import collections.vector

# TypeScript
import { Vector } from 'collections/vector';

# Rust
use collections::vector::Vector;

# Cb
import stdlib.std.vector;
```

### 4. エディタサポート
- オートコンプリートが容易
- リファクタリングツールとの親和性

## 制限事項

### 相対パスimport
```cb
// 現在サポート
import module_name;        // module_name.cb
import utils.helper;       // utils/helper.cb

// 未サポート（将来的に検討）
import ..utils.helper;     // ../utils/helper.cb
```

### 絶対パスimport
文字列リテラル廃止により、絶対パス指定は不可能に。
```cb
// 不可能
import "/absolute/path/to/module.cb";  // ❌ パースエラー

// 代わりにモジュールパス形式を使用
import absolute.path.to.module;        // ✅
```

## 互換性

### 後方互換性
❌ **破壊的変更**

既存のコードで文字列リテラル形式を使用している場合、パースエラーになります。

### 移行手順
1. すべての`import "..."`を`import ...`に変更
2. `.cb`拡張子を削除
3. `/`を`.`に置換

**一括変換（sed）**:
```bash
# stdlib imports
find . -name "*.cb" -exec sed -i '' \
  's|import "stdlib/\([^"]*\)\.cb";|import stdlib.\1;|g; s|/|.|g' {} \;

# 相対パスimports
find . -name "*.cb" -exec sed -i '' \
  's|import "\([^"]*\)\.cb";|import \1;|g' {} \;

# コメント修正（//が..に変換された場合）
find . -name "*.cb" -exec sed -i '' 's|\.\.|//|g' {} \;
```

## 関連コミット
- `cc96723`: Remove string literal import syntax
- `a893b59`: Remove string literal import support from interpreter

## 参考資料
- `src/frontend/recursive_parser/parsers/statement_parser.cpp`
- `src/frontend/recursive_parser/recursive_parser.cpp`
- Python import system
- Rust module system
- TypeScript import/export
