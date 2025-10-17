# Cb言語 プリプロセッサ実装計画

**作成日**: 2025年10月13日  
**対象バージョン**: v0.11.0  
**方針**: C/C++の伝統的なコンパイルフローに従う  

---

## 📋 コンパイルフローの確認

### C/C++の標準的なコンパイルフロー

```
ソースファイル (*.c, *.cpp)
  ↓
┌─────────────────────────────────────┐
│ 【1. プリプロセッサ】               │
│  - #include の展開                  │
│  - #define の展開                   │
│  - #if/#else/#endif の評価          │
│  - コメントの削除                   │
└─────────────────────────────────────┘
  ↓
プリプロセス済みソースコード
  ↓
┌─────────────────────────────────────┐
│ 【2. 字句解析 (Lexer)】             │
│  - トークン化                       │
└─────────────────────────────────────┘
  ↓
トークン列
  ↓
┌─────────────────────────────────────┐
│ 【3. 構文解析 (Parser)】            │
│  - AST構築                          │
└─────────────────────────────────────┘
  ↓
AST (抽象構文木)
  ↓
┌─────────────────────────────────────┐
│ 【4. 意味解析】                     │
│  - 型チェック                       │
│  - スコープ解決                     │
└─────────────────────────────────────┘
  ↓
┌─────────────────────────────────────┐
│ 【5. コード生成/実行】              │
│  - インタープリタ実行               │
│  - またはネイティブコード生成       │
└─────────────────────────────────────┘
```

### Cb言語での採用フロー（推奨）

```
ソースファイル (*.cb)
  ↓
┌─────────────────────────────────────┐
│ 【1. プリプロセッサ】               │  ← 新規追加！
│  src/frontend/preprocessor/         │
│  - #define の展開                   │
│  - #if/#else/#endif の評価          │
│  - #include の展開（将来）          │
└─────────────────────────────────────┘
  ↓
プリプロセス済みソースコード (文字列)
  ↓
┌─────────────────────────────────────┐
│ 【2. 字句解析 (Lexer)】             │
│  src/frontend/recursive_parser/     │
│  recursive_lexer.cpp                │
└─────────────────────────────────────┘
  ↓
トークン列
  ↓
┌─────────────────────────────────────┐
│ 【3. 構文解析 (Parser)】            │
│  src/frontend/recursive_parser/     │
│  recursive_parser.cpp               │
└─────────────────────────────────────┘
  ↓
AST (抽象構文木)
  ↓
┌─────────────────────────────────────┐
│ 【4. インタープリタ実行】           │
│  src/backend/interpreter/           │
└─────────────────────────────────────┘
```

---

## 🗂️ ディレクトリ構造

### 新規作成するファイル

```
src/frontend/preprocessor/
  ├── preprocessor.h              # プリプロセッサのメインクラス
  ├── preprocessor.cpp            # 実装
  ├── macro_definition.h          # マクロ定義の構造体
  ├── macro_expander.h            # マクロ展開器
  ├── macro_expander.cpp          # 実装
  ├── directive_parser.h          # ディレクティブパーサー (#define, #if等)
  ├── directive_parser.cpp        # 実装
  └── README.md                   # プリプロセッサの説明

tests/cases/preprocessor/
  ├── simple_define.cb            # 単純な#define
  ├── function_macro.cb           # 関数形式マクロ
  ├── stringify.cb                # #演算子
  ├── conditional.cb              # #if/#else/#endif
  └── nested_macro.cb             # ネストしたマクロ

tests/integration/preprocessor/
  └── test_preprocessor.hpp       # 統合テスト
```

---

## 📦 クラス設計

### 1. Preprocessor クラス（メイン）

```cpp
// src/frontend/preprocessor/preprocessor.h
#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace CbPreprocessor {

class Preprocessor {
public:
    Preprocessor();
    ~Preprocessor();
    
    /**
     * ソースコードをプリプロセスする
     * @param source 元のソースコード
     * @param filename ファイル名（エラーメッセージ用）
     * @return プリプロセス済みのソースコード
     */
    std::string process(const std::string& source, 
                       const std::string& filename = "<stdin>");
    
    /**
     * デバッグ用: マクロ定義の一覧を取得
     */
    std::vector<std::string> getDefinedMacros() const;
    
    /**
     * エラーメッセージを取得
     */
    std::string getLastError() const;
    
    /**
     * エラーが発生したかチェック
     */
    bool hasError() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace CbPreprocessor
```

---

### 2. MacroDefinition 構造体

```cpp
// src/frontend/preprocessor/macro_definition.h
#pragma once
#include <string>
#include <vector>

namespace CbPreprocessor {

/**
 * マクロ定義の種類
 */
enum class MacroType {
    OBJECT_LIKE,    // #define PI 3.14
    FUNCTION_LIKE   // #define SQUARE(x) ((x) * (x))
};

/**
 * マクロ定義を表す構造体
 */
struct MacroDefinition {
    std::string name;                      // マクロ名
    MacroType type;                        // マクロの種類
    std::vector<std::string> parameters;   // パラメータリスト（関数形式の場合）
    std::string body;                      // マクロ本体
    bool is_variadic;                      // 可変長引数か（__VA_ARGS__）
    int line;                              // 定義された行番号
    std::string filename;                  // 定義されたファイル名
    
    MacroDefinition()
        : type(MacroType::OBJECT_LIKE)
        , is_variadic(false)
        , line(0) {}
    
    bool isFunctionLike() const {
        return type == MacroType::FUNCTION_LIKE;
    }
};

} // namespace CbPreprocessor
```

---

### 3. MacroExpander クラス

```cpp
// src/frontend/preprocessor/macro_expander.h
#pragma once
#include "macro_definition.h"
#include <string>
#include <unordered_map>

namespace CbPreprocessor {

/**
 * マクロ展開を行うクラス
 */
class MacroExpander {
public:
    MacroExpander();
    
    /**
     * マクロを定義する
     */
    void define(const MacroDefinition& macro);
    
    /**
     * マクロが定義されているか確認
     */
    bool isDefined(const std::string& name) const;
    
    /**
     * マクロを未定義にする（#undef用）
     */
    void undefine(const std::string& name);
    
    /**
     * マクロを展開する
     * @param name マクロ名
     * @param args 引数リスト（関数形式の場合）
     * @return 展開後の文字列
     */
    std::string expand(const std::string& name,
                      const std::vector<std::string>& args = {});
    
    /**
     * ソースコード内のマクロをすべて展開
     * @param source ソースコード
     * @return 展開後のソースコード
     */
    std::string expandAll(const std::string& source);
    
    /**
     * 定義済みマクロの一覧を取得
     */
    std::vector<std::string> getDefinedMacros() const;
    
private:
    std::unordered_map<std::string, MacroDefinition> macros_;
    
    // #演算子（文字列化）を処理
    std::string stringifyArgument(const std::string& arg);
    
    // ##演算子（トークン結合）を処理
    std::string concatenateTokens(const std::string& left,
                                  const std::string& right);
    
    // マクロ展開を再帰的に実行（ネストしたマクロ対応）
    std::string expandRecursive(const std::string& text,
                                int depth = 0);
};

} // namespace CbPreprocessor
```

---

### 4. DirectiveParser クラス

```cpp
// src/frontend/preprocessor/directive_parser.h
#pragma once
#include <string>
#include <vector>

namespace CbPreprocessor {

/**
 * プリプロセッサディレクティブを解析するクラス
 */
class DirectiveParser {
public:
    /**
     * ディレクティブ行を解析
     * @param line ディレクティブ行（#define, #if等）
     * @return 処理が成功したか
     */
    bool parseLine(const std::string& line);
    
    /**
     * #defineディレクティブを解析
     */
    MacroDefinition parseDefine(const std::string& line);
    
    /**
     * #ifディレクティブの条件式を評価
     */
    bool evaluateCondition(const std::string& condition);
    
    /**
     * 関数形式マクロの引数を解析
     * 例: SQUARE(5) → ["5"]
     * 例: MAX(a+b, c*d) → ["a+b", "c*d"]
     */
    std::vector<std::string> parseArguments(const std::string& args_str);
    
private:
    // トークンを分割
    std::vector<std::string> tokenize(const std::string& str);
    
    // 括弧のバランスをチェック
    bool isBalanced(const std::string& str);
};

} // namespace CbPreprocessor
```

---

## 🔧 実装の詳細

### Phase 1: 基本的なプリプロセッサ（Week 1）

#### 1.1 Preprocessorクラスの実装

```cpp
// src/frontend/preprocessor/preprocessor.cpp
#include "preprocessor.h"
#include "macro_expander.h"
#include "directive_parser.h"
#include <sstream>
#include <iostream>

namespace CbPreprocessor {

class Preprocessor::Impl {
public:
    MacroExpander expander;
    DirectiveParser parser;
    std::string last_error;
    bool has_error = false;
    
    std::string process(const std::string& source, 
                       const std::string& filename) {
        std::istringstream input(source);
        std::ostringstream output;
        std::string line;
        int line_number = 0;
        
        while (std::getline(input, line)) {
            line_number++;
            
            // ディレクティブ行かチェック
            if (isDirectiveLine(line)) {
                if (!processDirective(line, line_number, filename)) {
                    return "";  // エラー
                }
            } else {
                // 通常の行: マクロを展開
                std::string expanded = expander.expandAll(line);
                output << expanded << "\n";
            }
        }
        
        return output.str();
    }
    
private:
    bool isDirectiveLine(const std::string& line) {
        // 先頭の空白をスキップ
        size_t pos = line.find_first_not_of(" \t");
        if (pos == std::string::npos) {
            return false;
        }
        return line[pos] == '#';
    }
    
    bool processDirective(const std::string& line, 
                         int line_number,
                         const std::string& filename) {
        // #define を処理
        if (line.find("#define") != std::string::npos) {
            try {
                MacroDefinition macro = parser.parseDefine(line);
                macro.line = line_number;
                macro.filename = filename;
                expander.define(macro);
                return true;
            } catch (const std::exception& e) {
                last_error = std::string("Error at line ") + 
                            std::to_string(line_number) + ": " + e.what();
                has_error = true;
                return false;
            }
        }
        
        // #undef を処理
        if (line.find("#undef") != std::string::npos) {
            size_t pos = line.find("undef") + 5;
            std::string name = line.substr(pos);
            // 空白を削除
            name.erase(0, name.find_first_not_of(" \t"));
            name.erase(name.find_last_not_of(" \t") + 1);
            expander.undefine(name);
            return true;
        }
        
        // その他のディレクティブは今後実装
        return true;
    }
};

Preprocessor::Preprocessor() 
    : impl_(std::make_unique<Impl>()) {}

Preprocessor::~Preprocessor() = default;

std::string Preprocessor::process(const std::string& source,
                                 const std::string& filename) {
    return impl_->process(source, filename);
}

std::vector<std::string> Preprocessor::getDefinedMacros() const {
    return impl_->expander.getDefinedMacros();
}

std::string Preprocessor::getLastError() const {
    return impl_->last_error;
}

bool Preprocessor::hasError() const {
    return impl_->has_error;
}

} // namespace CbPreprocessor
```

---

#### 1.2 main.cpp への統合

```cpp
// src/main.cpp の修正
#include "frontend/preprocessor/preprocessor.h"
#include "frontend/recursive_parser/recursive_lexer.h"
#include "frontend/recursive_parser/recursive_parser.h"

int main(int argc, char* argv[]) {
    // コマンドライン引数の処理
    bool preprocess_only = false;
    std::string filename;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-E" || arg == "--preprocess") {
            preprocess_only = true;
        } else {
            filename = arg;
        }
    }
    
    if (filename.empty()) {
        std::cerr << "Usage: " << argv[0] << " [-E] <file.cb>" << std::endl;
        return 1;
    }
    
    // ファイルを読み込む
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return 1;
    }
    
    std::string source((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();
    
    // ===== プリプロセッサを実行 =====
    CbPreprocessor::Preprocessor preprocessor;
    std::string preprocessed_source = preprocessor.process(source, filename);
    
    if (preprocessor.hasError()) {
        std::cerr << preprocessor.getLastError() << std::endl;
        return 1;
    }
    
    // -Eオプション: プリプロセス結果のみ出力
    if (preprocess_only) {
        std::cout << "# Preprocessed output from: " << filename << std::endl;
        std::cout << preprocessed_source << std::endl;
        return 0;
    }
    
    // ===== 以降は既存の処理 =====
    // Lexer → Parser → Interpreter
    RecursiveParserNS::RecursiveLexer lexer(preprocessed_source);
    RecursiveParserNS::RecursiveParser parser(lexer);
    
    ASTNode* ast = parser.parse();
    if (!ast) {
        std::cerr << "Parse error" << std::endl;
        return 1;
    }
    
    // インタープリタ実行
    Interpreter interpreter;
    interpreter.execute(ast);
    
    delete ast;
    return 0;
}
```

---

## 🧪 テストケース

### テスト1: 単純なマクロ定義

```cb
// tests/cases/preprocessor/simple_define.cb
#define PI 3.14159
#define TRUE 1
#define FALSE 0

int main() {
    println("PI =", PI);
    println("TRUE =", TRUE);
    println("FALSE =", FALSE);
    return 0;
}
```

**プリプロセス後**（`cb -E simple_define.cb`）:
```cb
int main() {
    println("PI =", 3.14159);
    println("TRUE =", 1);
    println("FALSE =", 0);
    return 0;
}
```

---

### テスト2: 関数形式マクロ

```cb
// tests/cases/preprocessor/function_macro.cb
#define SQUARE(x) ((x) * (x))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

int main() {
    int x = 5;
    int y = SQUARE(x);
    println("SQUARE(5) =", y);
    
    int max_val = MAX(10, 20);
    println("MAX(10, 20) =", max_val);
    
    return 0;
}
```

**プリプロセス後**:
```cb
int main() {
    int x = 5;
    int y = ((x) * (x));
    println("SQUARE(5) =", y);
    
    int max_val = ((10) > (20) ? (10) : (20));
    println("MAX(10, 20) =", max_val);
    
    return 0;
}
```

---

### テスト3: 文字列化演算子

```cb
// tests/cases/preprocessor/stringify.cb
#define STRINGIFY(x) #x
#define DEBUG_PRINT(expr) println(#expr " = " expr)

int main() {
    int count = 42;
    
    println(STRINGIFY(count));  // "count"
    DEBUG_PRINT(count);          // "count = 42"
    
    return 0;
}
```

---

## 📊 実装スケジュール

### Week 1: 基本プリプロセッサ

**ゴール**: 単純な`#define`が動作する

**タスク**:
- ✅ ディレクトリ構造の作成
- ✅ `Preprocessor`クラスの実装
- ✅ `MacroExpander`クラスの実装（基本）
- ✅ `DirectiveParser`クラスの実装（基本）
- ✅ `main.cpp`への統合
- ✅ テストケース作成

**成果物**:
```cb
#define PI 3.14159
int main() {
    println(PI);  // → println(3.14159);
    return 0;
}
```

---

### Week 2: 関数形式マクロ

**ゴール**: 引数を持つマクロが動作する

**タスク**:
- ✅ 引数解析の実装
- ✅ 引数置換の実装
- ✅ 括弧のバランスチェック
- ✅ ネストした引数対応

**成果物**:
```cb
#define SQUARE(x) ((x) * (x))
int result = SQUARE(5);  // → ((5) * (5))
```

---

### Week 3: プリプロセッサ演算子

**ゴール**: `#`と`##`演算子が動作する

**タスク**:
- ✅ `#`演算子（文字列化）
- ✅ `##`演算子（トークン結合）
- ✅ デバッグマクロの実装

**成果物**:
```cb
#define DEBUG_PRINT(expr) println(#expr " = " expr)
DEBUG_PRINT(count);  // → println("count" " = " count);
```

---

### Week 4: 条件付きコンパイル

**ゴール**: `#if/#else/#endif`が動作する

**タスク**:
- ✅ `#if`ディレクティブ
- ✅ `#ifdef`/`#ifndef`
- ✅ `#else`/`#elif`
- ✅ `#endif`
- ✅ 条件式の評価

**成果物**:
```cb
#define DEBUG 1

#if DEBUG
    #define LOG(msg) println(msg)
#else
    #define LOG(msg)
#endif
```

---

## 🎯 Makefileの更新

```makefile
# プリプロセッサのソースファイルを追加
PREPROCESSOR_SOURCES = \
    src/frontend/preprocessor/preprocessor.cpp \
    src/frontend/preprocessor/macro_expander.cpp \
    src/frontend/preprocessor/directive_parser.cpp

PREPROCESSOR_OBJECTS = $(PREPROCESSOR_SOURCES:.cpp=.o)

# 既存のOBJECTSに追加
OBJECTS += $(PREPROCESSOR_OBJECTS)
```

---

## 📝 ドキュメント

### READMEの作成

```markdown
# Cb言語 プリプロセッサ

## 概要

Cb言語のプリプロセッサは、C/C++の伝統的なプリプロセッサと同様に、
ソースコードをパース前に処理します。

## サポートされるディレクティブ

- `#define` - マクロ定義
- `#undef` - マクロ未定義化
- `#if` / `#else` / `#endif` - 条件付きコンパイル
- `#ifdef` / `#ifndef` - マクロ定義チェック

## 使用例

### 単純なマクロ
\`\`\`cb
#define PI 3.14159
\`\`\`

### 関数形式マクロ
\`\`\`cb
#define SQUARE(x) ((x) * (x))
\`\`\`

### プリプロセス結果の確認
\`\`\`bash
cb -E myfile.cb
\`\`\`
```

---

## ✅ まとめ

### 採用する設計

1. **プリプロセッサをパース前に実行** ✅
   - C/C++の伝統的なフローに従う
   - AST作成前にテキスト処理を完了

2. **独立したモジュール** ✅
   - `src/frontend/preprocessor/` に分離
   - 関心の分離、保守性の向上

3. **デバッグ可能** ✅
   - `-E`オプションで展開結果を確認
   - エラーメッセージの明確化

4. **段階的実装** ✅
   - Week 1: 基本的な`#define`
   - Week 2: 関数形式マクロ
   - Week 3: `#`/`##`演算子
   - Week 4: 条件付きコンパイル

---

**この設計により、明確で保守可能なプリプロセッサシステムが実現します！** 🚀
