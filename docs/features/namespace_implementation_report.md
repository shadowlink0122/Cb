# Namespace機能実装完了報告 - v0.11.0

**実装日**: 2025年10月13日  
**ブランチ**: feature/v0.10.1  
**ステータス**: ✅ 完了

## 📋 実装概要

Cb言語にC++スタイルのnamespace機能を実装しました。これにより、大規模コードベースでの名前衝突を回避し、コードの構造化が可能になります。

## 🎯 実装された機能

### 1. **基本的なnamespace宣言**
```cb
namespace math {
    int add(int a, int b) {
        return a + b;
    }
}
```

### 2. **修飾名での関数呼び出し**
```cb
int result = math::add(1, 2);  // 3
```

### 3. **ネストされたnamespace**
```cb
namespace outer {
    namespace inner {
        int multiply(int a, int b) {
            return a * b;
        }
    }
}

int result = outer::inner::multiply(3, 4);  // 12
```

### 4. **using namespace文**
```cb
namespace math {
    int add(int a, int b) { return a + b; }
}

using namespace math;

int main() {
    int sum = add(1, 2);  // 修飾名なしで呼び出し可能
    return 0;
}
```

### 5. **複数のusing namespace**
```cb
namespace math { int add(int a, int b) { return a + b; } }
namespace utils { int max(int a, int b) { return a > b ? a : b; } }

using namespace math;
using namespace utils;

int main() {
    int sum = add(1, 2);    // math::add
    int maximum = max(5, 3); // utils::max
    return 0;
}
```

### 6. **名前衝突の検出**
```cb
namespace math { int calc(int a, int b) { return a + b; } }
namespace physics { int calc(int a, int b) { return a * b; } }

using namespace math;
using namespace physics;

int main() {
    int result = calc(2, 3);  // エラー: 曖昧な参照
    return 0;
}
```
エラーメッセージ: `Ambiguous function call: 'calc' found in multiple namespaces`

### 7. **修飾名による曖昧さの解決**
```cb
namespace math { int calc(int a, int b) { return a + b; } }
namespace physics { int calc(int a, int b) { return a * b; } }

using namespace math;
using namespace physics;

int main() {
    int sum = math::calc(2, 3);      // 5 (明確)
    int product = physics::calc(2, 3); // 6 (明確)
    return 0;
}
```

## 🏗️ アーキテクチャ

### Lexer/Parser層
- **新規トークン**:
  - `TOK_NAMESPACE` (98)
  - `TOK_USING` (99)
  - `TOK_SCOPE_RESOLUTION` (100+) - `::`演算子
- **パーサー機能**:
  - `parseNamespace()` - namespace宣言のパース
  - `parseUsing()` - using文のパース
  - 複数の`::`をサポート (`outer::inner::multiply`)
- **ASTノード**:
  - `AST_NAMESPACE_DECL` (62)
  - `AST_USING_STMT` (63)

### NamespaceRegistry (新規コンポーネント)
**場所**: `src/backend/interpreter/core/namespace_registry.{h,cpp}`

**主要メソッド**:
```cpp
class NamespaceRegistry {
public:
    // namespace管理
    void registerNamespace(const std::string &ns_path, 
                          const ASTNode *declaration, 
                          bool is_exported);
    void enterNamespace(const std::string &ns_path);
    void exitNamespace();
    std::string getCurrentNamespace() const;
    
    // シンボル管理
    void registerSymbol(const std::string &name, ASTNode *declaration);
    ResolvedSymbol *resolveQualifiedName(const std::string &qualified_name) const;
    std::vector<ResolvedSymbol> resolveName(const std::string &name) const;
    
    // using namespace管理
    void addUsingNamespace(const std::string &ns_path);
    bool namespaceExists(const std::string &ns_path) const;
};
```

**データ構造**:
```cpp
struct NamespaceInfo {
    std::string path;                          // 完全修飾パス
    std::map<std::string, ASTNode*> symbols;   // シンボルテーブル
    const ASTNode *declaration;                // namespace宣言ノード
    bool is_exported;                          // exportフラグ
};

struct ResolvedSymbol {
    std::string namespace_path;  // 解決されたnamespace
    std::string symbol_name;     // シンボル名
    ASTNode *declaration;        // 宣言ノード
};
```

### Interpreter統合
**場所**: `src/backend/interpreter/core/interpreter.{h,cpp}`

**新規メソッド**:
```cpp
void handle_namespace_declaration(const ASTNode *node);
void handle_using_statement(const ASTNode *node);
NamespaceRegistry *get_namespace_registry();
```

**execute_statement()への追加**:
```cpp
case ASTNodeType::AST_NAMESPACE_DECL:
    handle_namespace_declaration(node);
    break;

case ASTNodeType::AST_USING_STMT:
    handle_using_statement(node);
    break;
```

### 関数宣言ハンドラ
**場所**: `src/backend/interpreter/handlers/declarations/function.cpp`

**変更内容**:
- namespace内で宣言された関数を完全修飾名で登録
- 例: `outer::inner::multiply`として`global_scope.functions`に登録
- `NamespaceRegistry`にもシンボルとして登録

```cpp
void FunctionDeclarationHandler::handle_function_declaration(const ASTNode *node) {
    auto* registry = interpreter_->get_namespace_registry();
    std::string function_key = node->name;
    
    if (registry) {
        std::string current_ns = registry->getCurrentNamespace();
        if (!current_ns.empty()) {
            function_key = current_ns + "::" + node->name;
            registry->registerSymbol(node->name, const_cast<ASTNode *>(node));
        }
    }
    
    interpreter_->global_scope.functions[function_key] = const_cast<ASTNode *>(node);
}
```

### 関数呼び出し評価
**場所**: `src/backend/interpreter/evaluator/functions/call_impl.cpp`

**検索順序**:
1. **修飾名検索** (`::`を含む場合)
   - `global_scope.functions`で完全修飾名を検索
   - 例: `outer::inner::multiply`
   
2. **通常の関数検索** (`::`を含まない場合)
   - まず`global_scope.functions`で非修飾名を検索
   - 見つからない場合、`using namespace`でインポートされたnamespaceから検索
   - `NamespaceRegistry::resolveName()`を使用
   
3. **曖昧性チェック**
   - 複数のnamespaceに同じ名前の関数がある場合、エラー
   - エラーメッセージ: `Ambiguous function call: 'func' found in multiple namespaces`

```cpp
// using namespaceからの検索
if (!func) {
    auto *registry = interpreter_.get_namespace_registry();
    if (registry) {
        std::vector<ResolvedSymbol> candidates = registry->resolveName(node->name);
        
        if (candidates.size() == 1) {
            func = candidates[0].declaration;
        } else if (candidates.size() > 1) {
            throw std::runtime_error("Ambiguous function call: '" + 
                                   node->name + "' found in multiple namespaces");
        }
    }
}
```

## ✅ テスト結果

### 基本機能テスト
| テストファイル | 説明 | 結果 |
|---------------|------|------|
| `empty_namespace.cb` | 空のnamespace宣言 | ✅ PASS |
| `namespace_with_function.cb` | namespace内の関数定義 | ✅ PASS |
| `qualified_call.cb` | 修飾名での関数呼び出し | ✅ PASS (出力: 3) |
| `nested_namespace.cb` | ネストされたnamespace | ✅ PASS (出力: 12) |

### using namespace テスト
| テストファイル | 説明 | 結果 |
|---------------|------|------|
| `using_namespace.cb` | 単一using namespace | ✅ PASS (出力: 312) |
| `multiple_using.cb` | 複数のusing namespace | ✅ PASS (出力: 155) |

### 名前衝突テスト
| テストファイル | 説明 | 結果 |
|---------------|------|------|
| `ambiguous_call.cb` | 名前衝突検出 | ✅ PASS (エラー検出) |
| `resolve_ambiguity.cb` | 修飾名で曖昧さ解決 | ✅ PASS (出力: 56) |

### 包括的テスト
| テストファイル | 説明 | 結果 |
|---------------|------|------|
| `comprehensive.cb` | 全機能統合テスト | ✅ PASS (出力: 81220158) |

### 既存テストスイート
- **統合テスト**: 2935個 - ✅ 全てPASS
- **ユニットテスト**: 50個 - ✅ 全てPASS
- **総計**: 2985個のテスト - ✅ 全てPASS

## 📊 実装統計

### 新規ファイル
- `src/backend/interpreter/core/namespace_registry.h` (152行)
- `src/backend/interpreter/core/namespace_registry.cpp` (237行)

### 変更ファイル
- `src/frontend/recursive_parser/recursive_lexer.h` - トークン定義追加
- `src/frontend/recursive_parser/recursive_lexer.cpp` - `::`のトークン化
- `src/frontend/recursive_parser/parsers/declaration_parser.cpp` - namespace/usingパース
- `src/frontend/recursive_parser/parsers/primary_expression_parser.cpp` - 複数`::`対応
- `src/backend/interpreter/core/interpreter.h` - NamespaceRegistry統合
- `src/backend/interpreter/core/interpreter.cpp` - namespace処理追加
- `src/backend/interpreter/handlers/declarations/function.cpp` - 完全修飾名登録
- `src/backend/interpreter/evaluator/functions/call_impl.cpp` - namespace対応検索
- `src/common/ast.h` - AST_NAMESPACE_DECL, AST_USING_STMT追加

### コード追加量
- **新規追加**: ~400行
- **既存修正**: ~150行
- **合計**: ~550行

## 🔧 技術的な課題と解決策

### 1. **NamespaceInfo のメモリ管理**
**課題**: `std::map::emplace()`でNamespaceInfoをコピーする際にSegfault

**解決策**: ポインタベースの管理に変更
```cpp
// 前: std::map<std::string, NamespaceInfo> namespaces_;
// 後: std::map<std::string, NamespaceInfo*> namespaces_;
```

### 2. **TOK_SCOPE vs TOK_SCOPE_RESOLUTION**
**課題**: TOK_SCOPEを削除すると既存のenum値が変わってしまう

**解決策**: 
- TOK_SCOPEを非推奨として残す
- 新しいTOK_SCOPE_RESOLUTIONを追加
- 全ての使用箇所を新トークンに移行

### 3. **execute_statement()でのnamespace処理**
**課題**: namespace宣言がdefaultケースに落ちてexpression evaluatorに渡されていた

**解決策**: `execute_statement()`のswitch文に明示的なケース追加
```cpp
case ASTNodeType::AST_NAMESPACE_DECL:
    handle_namespace_declaration(node);
    break;
```

### 4. **複数の::への対応**
**課題**: `outer::inner::multiply`のような2重以上の`::`がパースできない

**解決策**: whileループで複数の`::`を処理
```cpp
while (parser_->check(TokenType::TOK_SCOPE_RESOLUTION)) {
    parser_->advance();
    qualified_name += "::" + parser_->current_token_.value;
    parser_->advance();
}
```

## 🚀 今後の拡張可能性

### 実装済み ✅
- [x] 基本的なnamespace宣言
- [x] 修飾名での関数呼び出し
- [x] ネストされたnamespace
- [x] using namespace文
- [x] 名前衝突検出
- [x] 複数using namespaceのサポート

### 将来的な拡張 🔮
- [ ] namespace aliasing: `using ns = very::long::namespace::name;`
- [ ] 匿名namespace: `namespace { ... }`
- [ ] inline namespace: `inline namespace v2 { ... }`
- [ ] namespace内の構造体・型定義のサポート
- [ ] export namespace (モジュールシステムとの統合)
- [ ] ADL (Argument-Dependent Lookup)

## 📝 使用例

### 基本的な使い方
```cb
// 数学関数のnamespace
namespace math {
    int add(int a, int b) {
        return a + b;
    }
    
    int multiply(int a, int b) {
        return a * b;
    }
}

int main() {
    // 修飾名で呼び出し
    int sum = math::add(10, 20);        // 30
    int product = math::multiply(5, 6);  // 30
    
    print(sum);
    print(product);
    return 0;
}
```

### using namespaceを使った簡潔な記述
```cb
namespace math {
    int add(int a, int b) { return a + b; }
    int multiply(int a, int b) { return a * b; }
}

using namespace math;

int main() {
    // 修飾名なしで呼び出し
    int sum = add(10, 20);
    int product = multiply(5, 6);
    return 0;
}
```

### ネストされたnamespaceでのコード構造化
```cb
namespace company {
    namespace math {
        int add(int a, int b) { return a + b; }
    }
    
    namespace utils {
        int max(int a, int b) { return a > b ? a : b; }
    }
}

int main() {
    int sum = company::math::add(5, 3);      // 8
    int maximum = company::utils::max(10, 7); // 10
    return 0;
}
```

## 🎓 設計思想

### 1. **C++との互換性**
C++のnamespace構文に可能な限り近い構文を採用し、C++開発者にとって直感的な仕様としました。

### 2. **段階的な名前解決**
1. 完全修飾名
2. 非修飾名（グローバル）
3. using namespaceでインポートされたnamespace

この順序により、明示的な指定が優先され、予測可能な動作を実現しています。

### 3. **曖昧性の積極的な検出**
複数のnamespaceに同じ名前のシンボルがある場合、コンパイル時（実行時）にエラーとして検出し、バグの早期発見を支援します。

### 4. **拡張性**
NamespaceRegistryを独立したコンポーネントとして実装することで、将来的な機能拡張（型定義、変数、aliasing等）に対応しやすい設計としました。

## 📚 関連ドキュメント

- [BNF文法定義](../BNF.md) - namespace構文の追加
- [アーキテクチャ設計](../architecture/interpreter_structure.md) - NamespaceRegistry統合
- [実装ロードマップ](../todo/v0.11.0_implementation_plan.md)

## 🎉 結論

v0.11.0のnamespace機能実装により、Cb言語は大規模プロジェクトでのコード構造化能力を大幅に向上させました。

**主要な成果**:
- ✅ C++スタイルのnamespace構文
- ✅ ネストされたnamespace対応
- ✅ using namespace文による柔軟な名前解決
- ✅ 名前衝突の自動検出
- ✅ 全2985個のテストが通過
- ✅ 既存機能への影響なし

この実装により、Cbは教育用言語から実用的なプログラミング言語へと一歩前進しました。

---

**実装者**: GitHub Copilot + shadowlink  
**レビュー**: ✅ 完了  
**マージ準備**: ✅ Ready
