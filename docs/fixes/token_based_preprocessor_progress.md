# トークンベースプリプロセッサ実装記録

## Phase 1: レキサーの拡張 ✅ 完了

### 実装内容

#### 1. トークンタイプの追加
`src/frontend/recursive_parser/recursive_lexer.h`:
- `TOK_HASH` - `#` 単体
- `TOK_PREPROCESSOR_DEFINE` - `#define` ディレクティブ
- `TOK_PREPROCESSOR_UNDEF` - `#undef` ディレクティブ

#### 2. レキサーの拡張
`src/frontend/recursive_parser/recursive_lexer.cpp`:
- `makePreprocessorDirective()` メソッドを追加
- `#` で始まる行を検出してディレクティブ全体をトークン化
- `nextToken()` に `case '#'` を追加

#### 3. テストの追加
`tests/unit/frontend/recursive_parser/test_lexer_preprocessor.hpp`:
- `test_lexer_preprocessor_define` - #define のトークン化
- `test_lexer_preprocessor_undef` - #undef のトークン化  
- `test_lexer_preprocessor_with_spaces` - スペース込みのディレクティブ
- `test_lexer_string_literal_not_affected` - 文字列リテラルは影響を受けない
- `test_lexer_normal_hash` - 未知のディレクティブは TOK_HASH

### テスト結果

```
=== Lexer Preprocessor Tests ===
✓ test_lexer_preprocessor_define passed
✓ test_lexer_preprocessor_undef passed
✓ test_lexer_preprocessor_with_spaces passed
✓ test_lexer_string_literal_not_affected passed
✓ test_lexer_normal_hash passed
All lexer preprocessor tests passed!
```

全体のテスト: **54/54 tests passed** ✅

### 動作確認

```cpp
// 入力
#define PI 3.14159
int main() {}

// トークン列
Token { type: TOK_PREPROCESSOR_DEFINE, value: "#define PI 3.14159" }
Token { type: TOK_INT, value: "int" }
Token { type: TOK_IDENTIFIER, value: "main" }
...
```

## Phase 2: TokenPreprocessor の実装 (次のステップ)

### 設計方針

#### クラス構造
```cpp
class TokenPreprocessor {
public:
    std::vector<Token> process(const std::vector<Token>& tokens);
    
private:
    MacroExpander expander_;
    
    void processDefineDirective(const Token& directive);
    void processUndefDirective(const Token& directive);
    std::vector<Token> expandMacroInToken(const Token& token);
    bool shouldExpandToken(const Token& token);
};
```

#### 処理フロー
```
Input: std::vector<Token>
  ↓
For each token:
  - TOK_PREPROCESSOR_DEFINE → マクロ定義を登録、トークンを削除
  - TOK_PREPROCESSOR_UNDEF → マクロ定義を解除、トークンを削除
  - TOK_IDENTIFIER → マクロ展開を試みる
  - TOK_STRING → スキップ（展開しない）
  - その他 → そのまま出力
  ↓
Output: std::vector<Token> (展開済み)
```

### 実装計画

#### Step 1: TokenPreprocessor クラスの作成
- `src/frontend/preprocessor/token_preprocessor.h`
- `src/frontend/preprocessor/token_preprocessor.cpp`

#### Step 2: ディレクティブ処理の実装
- `processDefineDirective()` - ディレクティブトークンをパース
- `processUndefDirective()` - マクロ定義を解除

#### Step 3: マクロ展開の実装
- `expandMacroInToken()` - 識別子トークンをマクロ展開
- 関数マクロの場合は次のトークン（引数リスト）も処理

#### Step 4: main.cpp の統合
```cpp
// 新しいフロー
RecursiveLexer lexer(source);
std::vector<Token> tokens;
while (!lexer.isAtEnd()) {
    tokens.push_back(lexer.nextToken());
}

TokenPreprocessor preprocessor;
auto processedTokens = preprocessor.process(tokens);

RecursiveParser parser(processedTokens);
auto ast = parser.parse();
```

#### Step 5: RecursiveParser の変更
- 現在: `RecursiveParser(const std::string& source)` - 文字列を受け取る
- 新規: `RecursiveParser(const std::vector<Token>& tokens)` - トークン列を受け取る
- レキサーを内部で使わない

### テスト計画

#### Unit Tests
- `test_token_preprocessor.hpp`:
  - `test_process_define_directive()`
  - `test_process_undef_directive()`
  - `test_expand_object_macro()`
  - `test_expand_function_macro()`
  - `test_string_literal_not_expanded()`
  - `test_nested_macro_expansion()`

#### Integration Tests
既存のデモプログラムが動作することを確認：
- `tests/cases/preprocessor/macro_demo.cb`
- `tests/cases/preprocessor/function_macro_demo.cb`
- `tests/cases/preprocessor/nested_macro_demo.cb`

### 期待される効果

#### 問題の修正
1. ✅ 文字列リテラル内のマクロ展開を防ぐ
   ```cb
   #define PI 3.14159
   println("PI = ");  // → println("PI = "); (展開されない!)
   ```

2. ✅ 識別子の境界を明確に
   ```cb
   #define PI 3.14159
   int API = 100;  // → int API = 100; (TOK_IDENTIFIERが完全一致)
   ```

3. ✅ コメント内のマクロ展開を防ぐ
   ```cb
   #define MAX 100
   // MAX should not be expanded  // → (レキサーがコメントを除去)
   ```

### マイルストーン

- ✅ **Phase 1**: レキサーの拡張（完了）
  - プリプロセッサディレクティブのトークン化
  - 5つのテストケース追加
  - 54/54 tests passing

- 🔄 **Phase 2**: TokenPreprocessor の実装（次）
  - トークンベースのプリプロセッサクラス
  - main.cpp の統合
  - RecursiveParser の変更

- 📋 **Phase 3**: テストと検証
  - 文字列リテラルバグの修正確認
  - 既存のデモプログラムの動作確認
  - パフォーマンステスト

## 次のアクション

Phase 2 の実装を開始：
1. `TokenPreprocessor` クラスの作成
2. ディレクティブ処理の実装
3. マクロ展開ロジックの実装
