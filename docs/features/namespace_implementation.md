# Namespace & Using 実装設計書

## 概要

C++スタイルのnamespace機能を実装し、既存のimport/exportシステムと統合する。

## 仕様

### 1. Namespace宣言

```cb
// 基本的なnamespace宣言
namespace std {
    void println(string msg) {
        // 実装
    }
}

// ネストしたnamespace
namespace std {
    namespace io {
        void println(string msg) {
            // 実装
        }
    }
}

// 同一namespace内での関数呼び出しはnamespace修飾不要
namespace math {
    int add(int a, int b) {
        return a + b;
    }
    
    int mul(int a, int b) {
        return add(a, a) * b;  // math::add() と書く必要なし
    }
}
```

### 2. Export構文

```cb
// 方法1: export namespaceで一括エクスポート
export namespace std {
    void println(string msg) { }
}

// 方法2: namespace定義後にexport
namespace std {
    void println(string msg) { }
}
export std;

// 方法3: ネストしたnamespaceのexport
namespace io {
    void println(string msg) { }
}

export namespace std {
    export io;  // stdの中にioをネスト
}
```

### 3. Import構文

```cb
// ファイル/フォルダからのimport
import "path/to/file".std;           // std namespaceをimport
import "path/to/file".std::io;       // std::io namespaceをimport
import "path/to/file".std::io::*;    // std::ioのすべてをimport
import "path/to/file".std::io::println;  // 特定の関数のみimport

// 簡略形（現在のディレクトリまたは標準ライブラリから検索）
import std;           // std namespace/folder/fileを検索
import std::io;       // std::io namespace/file/folderを検索
import std::io::*;    // std::ioのすべてをimport
```

### 4. Using宣言

```cb
// using namespace: namespace修飾を省略
import std;
using namespace std;
io::println("Hello");  // std::io::println() が使える

// ネストしたnamespace
using namespace std::io;
println("Hello");  // std::io::println() が使える

// using namespace の制約
import std;
using namespace io;  // エラー: トップレベルにioがない
println();           // エラー

using namespace std::io;
println("Hello");     // OK
std::println();       // エラー: 中間のnamespaceは省略不可
```

### 5. 名前衝突の検出

`using namespace`による名前衝突はコンパイル時エラーとして検出されます。

```cb
// エラーケース1: using namespace後の名前衝突
namespace math {
    void add(int a, int b) { }
}

namespace calc {
    void add(int a, int b) { }
}

int main() {
    using namespace math;
    using namespace calc;
    add(1, 2);  // エラー: 'add'が曖昧（math::add と calc::add）
}
```

```cb
// エラーケース2: グローバル関数との衝突
void println(string msg) {
    // グローバル実装
}

namespace std {
    void println(string msg) {
        // namespace実装
    }
}

int main() {
    using namespace std;
    println("test");  // エラー: 'println'が曖昧（グローバルと std::println）
}
```

```cb
// エラーケース3: ローカル関数との衝突
namespace lib {
    void process() { }
}

int main() {
    void process() {  // ローカル関数
        println("local");
    }
    
    using namespace lib;
    process();  // OK: ローカル関数が優先（シャドーイング）
    lib::process();  // OK: 明示的な修飾で呼び分け可能
}
```

#### 名前衝突の解決優先順位

1. **ローカルスコープ**: 関数内で定義された変数・関数（最優先）
2. **現在のnamespace**: 同一namespace内の定義
3. **明示的修飾**: `namespace::name` 形式の呼び出し
4. **using namespace**: 複数のusing宣言で同じ名前があればエラー
5. **グローバルスコープ**: namespace外の定義

#### 衝突検出のタイミング

- **関数定義時**: 同一namespace内での重複定義を検出
- **using宣言時**: 既存のusing namespaceとの衝突を検出
- **関数呼び出し時**: 名前解決で複数の候補が見つかった場合にエラー

```cb
// 検出例1: 関数定義時の衝突
namespace test {
    void func() { }
    void func() { }  // エラー: 'func'が既に定義されている
}

// 検出例2: using宣言時の衝突チェック
namespace a { void func() { } }
namespace b { void func() { } }

int main() {
    using namespace a;
    using namespace b;  // 警告: 'func'が複数のnamespaceで定義されている
    // ここでfunc()を呼ぶとエラー
}

// 検出例3: 呼び出し時の曖昧性エラー
namespace x { void test() { } }
namespace y { void test() { } }

int main() {
    using namespace x;
    using namespace y;
    test();  // エラー: 'test'の呼び出しが曖昧
             // 候補: x::test, y::test
}
```

### 6. スコープ規則

#### 同一ファイル内

```cb
namespace math {
    int add(int a, int b) {
        return a + b;
    }
}

int main() {
    math::add(1, 2);  // OK: namespace修飾が必要
    add(1, 2);        // エラー: namespace修飾なし
    
    using namespace math;
    add(1, 2);        // OK: using宣言後は省略可能
}
```

#### namespace内から外部への参照

```cb
int global_var = 42;

namespace test {
    int local_var = 10;
    
    void func() {
        println(global_var);  // OK: グローバルスコープ
        println(local_var);   // OK: 同一namespace
    }
}
```

## 技術設計

### 1. トークン追加

```cpp
enum class TokenType {
    // ...
    TOK_NAMESPACE,  // namespace
    TOK_USING,      // using
    // ...
};
```

### 2. ASTノード追加

```cpp
enum ASTNodeType {
    // ...
    AST_NAMESPACE_DECL,  // namespace宣言
    AST_USING_STMT,      // using文
    // ...
};

struct ASTNode {
    // ...
    std::string namespace_name;              // namespace名
    std::vector<std::string> namespace_path; // namespace階層 (std::io::core)
    std::vector<ASTNode*> namespace_body;    // namespace内の定義
    bool is_using_namespace;                 // using namespace かどうか
    // ...
};
```

### 3. Namespace解決メカニズム

#### 名前解決の優先順序

1. **ローカルスコープ**（関数内変数・関数）- 最優先
2. **現在のnamespace内** - namespace内からの参照
3. **using namespace宣言** - 複数候補があれば曖昧性エラー
4. **グローバルスコープ** - namespace外の定義

#### Namespace修飾呼び出し

```cpp
// std::io::println() の解決
// 1. "std" namespaceを検索
// 2. "std"内の"io" namespaceを検索
// 3. "io"内の"println"を検索
```

#### 名前衝突の検出

```cpp
// 実装例: 名前解決時の衝突チェック
std::vector<ASTNode*> resolve_candidates(const std::string& name) {
    std::vector<ASTNode*> candidates;
    
    // 1. ローカルスコープを検索
    if (auto* local = find_local(name)) {
        return {local};  // ローカルが見つかれば即座に返す（最優先）
    }
    
    // 2. 現在のnamespaceを検索
    if (auto* current_ns = find_in_current_namespace(name)) {
        candidates.push_back(current_ns);
    }
    
    // 3. using namespace宣言されたnamespaceを検索
    for (auto* ns : active_using_namespaces_) {
        if (auto* found = find_in_namespace(ns, name)) {
            candidates.push_back(found);
        }
    }
    
    // 4. グローバルスコープを検索
    if (auto* global = find_global(name)) {
        candidates.push_back(global);
    }
    
    // 5. 候補が複数あれば曖昧性エラー
    if (candidates.size() > 1) {
        throw_ambiguous_reference_error(name, candidates);
    }
    
    return candidates;
}
```

### 4. Import/Exportとの統合

#### Import時のnamespace解決

```cb
// ファイル: math.cb
export namespace math {
    int add(int a, int b) { return a + b; }
}

// 使用側
import "math.cb".math;
math::add(1, 2);  // OK

import "math.cb".math::add;
add(1, 2);  // OK
```

#### フォルダとnamespaceの対応

```
stdlib/
  math/
    basic.cb    -> namespace math::basic { ... }
    advanced.cb -> namespace math::advanced { ... }
```

```cb
import "stdlib/math/basic.cb".math::basic;
// または
import math::basic;  // 自動的にファイル検索
```

## 実装手順

### Phase 1: Lexer/Parser拡張
1. `TOK_NAMESPACE`, `TOK_USING` トークン追加
2. `parse_namespace_declaration()` 実装
3. `parse_using_statement()` 実装
4. `::`演算子のパース強化（現在は既にある）

### Phase 2: AST拡張
1. `AST_NAMESPACE_DECL`, `AST_USING_STMT` ノード追加
2. namespace情報を保持する構造追加
3. 修飾名の表現（`std::io::println`）

### Phase 3: Interpreter拡張
1. Namespace登録・管理機構
2. 名前解決エンジンの実装
3. using namespace のスコープ管理
4. **名前衝突の検出とエラー報告** ⭐
5. Import/Export時のnamespace解決

### Phase 4: テスト
1. 基本的なnamespace宣言・呼び出し
2. ネストしたnamespace
3. using namespace宣言
4. **名前衝突の検出テスト** ⭐
   - using namespace間の衝突
   - グローバル関数との衝突
   - ローカルスコープの優先順位
5. Import/Export統合
6. スコープ規則のテスト

## 既存システムとの競合分析

### 現在のImport/Export

```cb
// 現在
import "file.cb";              // ファイル全体をimport
export func();                 // 関数をexport
export struct S { };           // 構造体をexport

// Namespace導入後も互換性維持
import "file.cb";              // 引き続き動作
export func();                 // 引き続き動作（暗黙的にグローバルnamespace）
```

### ファイルパス vs Namespace

```cb
// 曖昧性の例
import std;  // これは何？
// 1. "std.cb" ファイル？
// 2. "std/" フォルダ？
// 3. 現在のファイル内の "std" namespace？

// 解決策: 検索順序を定義
// 1. 現在のファイル内のnamespaceを検索
// 2. 同一ディレクトリのファイル/フォルダを検索
// 3. 標準ライブラリパスを検索
```

### `::`演算子の多重用途

```cb
// 既存: enum アクセス
enum Color { RED, GREEN }
Color::RED;

// 新規: namespace アクセス
namespace test { int x; }
test::x;

// 実装: 名前解決時に型を判定
```

## テストケース

### 基本的なnamespace

```cb
namespace test {
    int value = 42;
}

int main() {
    println(test::value);  // 42
}
```

### ネストしたnamespace

```cb
namespace outer {
    namespace inner {
        int value = 100;
    }
}

int main() {
    println(outer::inner::value);  // 100
}
```

### Using namespace

```cb
namespace math {
    int add(int a, int b) { return a + b; }
}

int main() {
    using namespace math;
    println(add(1, 2));  // 3
}
```

### Import/Export統合

```cb
// file1.cb
export namespace lib {
    int helper() { return 42; }
}

// file2.cb
import "file1.cb".lib;
int main() {
    println(lib::helper());  // 42
}
```

## 将来の拡張

1. **Namespace alias**: `namespace short = very::long::namespace::name;`
2. **Anonymous namespace**: `namespace { ... }` (ファイルスコープ)
3. **Inline namespace**: `inline namespace v2 { ... }`
4. **Using宣言の拡張**: `using math::add;` (関数単位)

## 関連ドキュメント

- `docs/spec.md` - 言語仕様書（更新予定）
- `docs/BNF.md` - 文法定義（更新予定）
- `release_notes/v0.11.0.md` - リリースノート（作成予定）
