# Phase 2: FFI基盤実装詳細

**作成日**: 2025-11-14  
**ステータス**: 実装準備完了  
**担当**: Phase 2 実装チーム

## 概要

Phase 2では、外部Cライブラリの関数を呼び出せるFFI（Foreign Function Interface）システムの基盤を実装します。

## 目標

1. 基本的なC関数（int, double, string）を呼び出せる
2. 型安全な関数宣言
3. エラーハンドリング
4. 最小限の実装で動作確認

## 実装の優先順位

### 🔴 必須（Phase 2で完了）

- [ ] レキサーに `foreign` キーワード追加
- [ ] `use foreign.module` 構文のパース
- [ ] FFIマネージャーの基本実装
- [ ] int/double/string型の変換
- [ ] dlopen/dlsym統合
- [ ] 基本的なテストケース（5個）

### 🟡 重要（Phase 3で実装）

- [ ] 構造体の受け渡し
- [ ] ポインタ型の完全サポート
- [ ] コールバック関数
- [ ] .cbfファイルサポート

### 🟢 将来（Phase 4以降）

- [ ] 可変長引数
- [ ] 複雑なデータ構造
- [ ] カスタム型マーシャリング

## 実装ステップ

### Step 1: レキサー拡張

**ファイル**: `src/frontend/recursive_parser/recursive_lexer.cpp`

**変更内容**:
```cpp
// キーワード追加
keywords["foreign"] = TokenType::KEYWORD;
keywords["lib"] = TokenType::KEYWORD;  // 将来用
```

**テスト**:
```cb
use foreign.math {
    int add(int a, int b);
}
```

### Step 2: ASTノード追加

**ファイル**: `src/common/ast.h`

**新規ノード**:
```cpp
// 外部関数宣言
struct ForeignFunctionDecl {
    std::string module_name;      // "math"
    std::string function_name;    // "add"
    std::string return_type;      // "int"
    std::vector<Parameter> params;
    int line;
};

// 外部モジュール宣言
struct ForeignModuleDecl {
    std::string module_name;
    std::vector<ForeignFunctionDecl> functions;
    int line;
};
```

**ASTNode拡張**:
```cpp
enum class ASTNodeType {
    // 既存...
    FOREIGN_MODULE_DECL,
    FOREIGN_FUNCTION_DECL,
};

struct ASTNode {
    // 既存...
    std::shared_ptr<ForeignModuleDecl> foreign_module_decl;
    std::shared_ptr<ForeignFunctionDecl> foreign_function_decl;
};
```

### Step 3: パーサー拡張

**ファイル**: `src/frontend/recursive_parser/parsers/declaration_parser.cpp`

**新規関数**:
```cpp
// use foreign.module { ... } のパース
ASTNode Parser::parseForeignModuleDecl() {
    // "use" トークンは既に消費済み
    expect("foreign", "Expected 'foreign' after 'use'");
    expect(".", "Expected '.' after 'foreign'");
    
    std::string module_name = expect(TokenType::IDENTIFIER, "Expected module name");
    
    expect("{", "Expected '{' to start foreign function declarations");
    
    std::vector<ForeignFunctionDecl> functions;
    while (!check("}")) {
        functions.push_back(parseForeignFunctionDecl());
        expect(";", "Expected ';' after function declaration");
    }
    
    expect("}", "Expected '}' to end foreign function declarations");
    
    // ASTノード作成
    auto node = createNode(ASTNodeType::FOREIGN_MODULE_DECL);
    node->foreign_module_decl = std::make_shared<ForeignModuleDecl>();
    node->foreign_module_decl->module_name = module_name;
    node->foreign_module_decl->functions = functions;
    
    return node;
}

// 関数宣言のパース（通常の関数宣言と同じ形式）
ForeignFunctionDecl Parser::parseForeignFunctionDecl() {
    ForeignFunctionDecl decl;
    
    // 戻り値の型
    decl.return_type = parseType();
    
    // 関数名
    decl.function_name = expect(TokenType::IDENTIFIER, "Expected function name");
    
    // 引数リスト
    expect("(", "Expected '(' after function name");
    
    while (!check(")")) {
        Parameter param;
        param.type = parseType();
        param.name = expect(TokenType::IDENTIFIER, "Expected parameter name");
        decl.params.push_back(param);
        
        if (!check(")")) {
            expect(",", "Expected ',' or ')' in parameter list");
        }
    }
    
    expect(")", "Expected ')' after parameters");
    
    return decl;
}
```

**既存パーサーの修正**:
```cpp
ASTNode Parser::parseUseStatement() {
    consume(); // "use"を消費
    
    // foreignキーワードチェック
    if (check("foreign")) {
        return parseForeignModuleDecl();
    }
    
    // 既存のuse構文処理
    // ...
}
```

### Step 4: FFIマネージャー実装

**ファイル**: `src/backend/interpreter/ffi/ffi_manager.h`

**クラス定義**:
```cpp
#pragma once
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <dlfcn.h>

namespace FFI {

// 関数シグネチャ
struct FunctionSignature {
    std::string return_type;
    std::vector<std::string> param_types;
};

// ロードされたライブラリ
struct LoadedLibrary {
    void* handle;
    std::string path;
    std::map<std::string, void*> functions;
};

class FFIManager {
public:
    FFIManager();
    ~FFIManager();
    
    // ライブラリのロード
    bool loadLibrary(const std::string& module_name, const std::string& library_path);
    
    // 関数の登録
    bool registerFunction(const std::string& module_name, 
                         const std::string& function_name,
                         const FunctionSignature& signature);
    
    // 関数の呼び出し
    Value callFunction(const std::string& module_name,
                      const std::string& function_name,
                      const std::vector<Value>& args);
    
    // ライブラリパスの解決
    std::string resolveLibraryPath(const std::string& module_name);
    
private:
    std::map<std::string, LoadedLibrary> loaded_libraries_;
    std::map<std::string, FunctionSignature> function_signatures_;
    
    // ライブラリ検索パス
    std::vector<std::string> search_paths_;
    
    // 型変換ヘルパー
    void* convertToCType(const Value& val, const std::string& type);
    Value convertFromCType(void* ptr, const std::string& type);
};

} // namespace FFI
```

**実装ファイル**: `src/backend/interpreter/ffi/ffi_manager.cpp`

```cpp
#include "ffi_manager.h"
#include <iostream>
#include <filesystem>
#include <ffi.h>  // libffiを使用

namespace FFI {

FFIManager::FFIManager() {
    // 検索パス初期化
    search_paths_.push_back("./stdlib/foreign/");
    search_paths_.push_back("/usr/local/lib/");
    search_paths_.push_back("/usr/lib/");
    
    #ifdef __APPLE__
    search_paths_.push_back("/opt/homebrew/lib/");
    #endif
}

FFIManager::~FFIManager() {
    // 全ライブラリをアンロード
    for (auto& [name, lib] : loaded_libraries_) {
        if (lib.handle) {
            dlclose(lib.handle);
        }
    }
}

bool FFIManager::loadLibrary(const std::string& module_name, 
                             const std::string& library_path) {
    // 既にロード済みかチェック
    if (loaded_libraries_.find(module_name) != loaded_libraries_.end()) {
        return true;
    }
    
    // ライブラリパス解決
    std::string full_path = resolveLibraryPath(library_path);
    if (full_path.empty()) {
        std::cerr << "Error: Could not find library: " << library_path << std::endl;
        return false;
    }
    
    // ライブラリロード
    void* handle = dlopen(full_path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        std::cerr << "Error loading library: " << dlerror() << std::endl;
        return false;
    }
    
    // 登録
    LoadedLibrary lib;
    lib.handle = handle;
    lib.path = full_path;
    loaded_libraries_[module_name] = lib;
    
    std::cout << "[FFI] Loaded library: " << module_name 
              << " from " << full_path << std::endl;
    
    return true;
}

bool FFIManager::registerFunction(const std::string& module_name,
                                  const std::string& function_name,
                                  const FunctionSignature& signature) {
    // ライブラリがロードされているかチェック
    auto lib_it = loaded_libraries_.find(module_name);
    if (lib_it == loaded_libraries_.end()) {
        std::cerr << "Error: Library not loaded: " << module_name << std::endl;
        return false;
    }
    
    // 関数シンボル取得
    void* func_ptr = dlsym(lib_it->second.handle, function_name.c_str());
    if (!func_ptr) {
        std::cerr << "Error: Function not found: " << function_name 
                  << " in " << module_name << std::endl;
        std::cerr << dlerror() << std::endl;
        return false;
    }
    
    // 関数ポインタを登録
    lib_it->second.functions[function_name] = func_ptr;
    
    // シグネチャを登録
    std::string full_name = module_name + "." + function_name;
    function_signatures_[full_name] = signature;
    
    std::cout << "[FFI] Registered function: " << full_name << std::endl;
    
    return true;
}

Value FFIManager::callFunction(const std::string& module_name,
                               const std::string& function_name,
                               const std::vector<Value>& args) {
    // 関数ポインタ取得
    auto lib_it = loaded_libraries_.find(module_name);
    if (lib_it == loaded_libraries_.end()) {
        throw std::runtime_error("Library not loaded: " + module_name);
    }
    
    auto func_it = lib_it->second.functions.find(function_name);
    if (func_it == lib_it->second.functions.end()) {
        throw std::runtime_error("Function not found: " + function_name);
    }
    
    void* func_ptr = func_it->second;
    
    // シグネチャ取得
    std::string full_name = module_name + "." + function_name;
    auto sig_it = function_signatures_.find(full_name);
    if (sig_it == function_signatures_.end()) {
        throw std::runtime_error("Function signature not found: " + full_name);
    }
    
    const FunctionSignature& sig = sig_it->second;
    
    // TODO: libffiを使った実際の関数呼び出し
    // これは次のステップで実装
    
    return Value();  // 仮の実装
}

std::string FFIManager::resolveLibraryPath(const std::string& module_name) {
    // プラットフォーム別の拡張子
    #ifdef __APPLE__
    std::string extension = ".dylib";
    #elif defined(_WIN32)
    std::string extension = ".dll";
    #else
    std::string extension = ".so";
    #endif
    
    // ライブラリ名の候補
    std::vector<std::string> candidates = {
        module_name,
        "lib" + module_name + extension,
        module_name + extension
    };
    
    // 検索パスを探索
    for (const auto& search_path : search_paths_) {
        for (const auto& candidate : candidates) {
            std::string full_path = search_path + candidate;
            if (std::filesystem::exists(full_path)) {
                return full_path;
            }
        }
    }
    
    return "";
}

} // namespace FFI
```

### Step 5: インタプリタ統合

**ファイル**: `src/backend/interpreter/core/interpreter.h`

**追加メンバー**:
```cpp
#include "../ffi/ffi_manager.h"

class Interpreter {
    // 既存...
    
private:
    std::unique_ptr<FFI::FFIManager> ffi_manager_;
    
    // 外部モジュール宣言の処理
    void handleForeignModuleDecl(const ASTNode& node);
};
```

**実装**: `src/backend/interpreter/handlers/declarations/foreign.cpp`

```cpp
#include "../../core/interpreter.h"

void Interpreter::handleForeignModuleDecl(const ASTNode& node) {
    auto& decl = node.foreign_module_decl;
    
    // ライブラリロード
    std::string library_path = decl->module_name;
    if (!ffi_manager_->loadLibrary(decl->module_name, library_path)) {
        throw std::runtime_error("Failed to load foreign library: " + decl->module_name);
    }
    
    // 各関数を登録
    for (const auto& func : decl->functions) {
        FFI::FunctionSignature sig;
        sig.return_type = func.return_type;
        
        for (const auto& param : func.params) {
            sig.param_types.push_back(param.type);
        }
        
        if (!ffi_manager_->registerFunction(decl->module_name, 
                                           func.function_name, 
                                           sig)) {
            std::cerr << "Warning: Failed to register function: " 
                      << func.function_name << std::endl;
        }
    }
}
```

### Step 6: テストケース作成

**ファイル**: `tests/cases/ffi/basic_math.cb`

```cb
// テスト: 基本的な数学関数
use foreign.m {
    double sqrt(double x);
    double pow(double x, double y);
}

void main() {
    double s = sqrt(16.0);
    println(s);  // 4.0
    
    double p = pow(2.0, 3.0);
    println(p);  // 8.0
}
```

## ビルド設定

**Makefile追加**:
```makefile
# FFI サポート
FFI_DIR=$(INTERPRETER_DIR)/ffi
FFI_SOURCES=$(wildcard $(FFI_DIR)/*.cpp)
FFI_OBJECTS=$(FFI_SOURCES:.cpp=.o)

# libffi リンク (必要な場合)
LDFLAGS += -lffi -ldl
```

## 依存関係

### 必須

- `libdl` (dynamic linking)
- C++17標準ライブラリ

### オプション

- `libffi` (より高度な型変換に必要)

## テスト計画

1. **基本テスト**: シンプルなint関数
2. **double型テスト**: 浮動小数点数
3. **文字列テスト**: char*の受け渡し
4. **複数引数テスト**: 3つ以上の引数
5. **エラーテスト**: 存在しない関数、型不一致

## 完了条件

- [ ] 上記5つのテストケースが成功
- [ ] エラーメッセージが適切
- [ ] メモリリークなし（valgrind確認）
- [ ] ドキュメント作成

## 次のPhase（Phase 3）への移行条件

1. Phase 2の全テストがパス
2. 基本的なC関数呼び出しが安定動作
3. エラーハンドリングが適切

## 参考資料

- [libffi documentation](https://sourceware.org/libffi/)
- [dlopen man page](https://man7.org/linux/man-pages/man3/dlopen.3.html)
- `modern_ffi_macro_design.md` - FFI設計詳細
