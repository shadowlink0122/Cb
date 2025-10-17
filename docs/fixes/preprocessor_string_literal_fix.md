# プリプロセッサ文字列リテラル展開バグ修正

## 問題点

### 1. 文字列リテラル内のマクロ展開
```cb
#define PI 3.14159
println("PI = ");  // 期待: "PI = "  実際: "3.14159 = "
```

### 2. 識別子の部分一致（現在は問題なし）
```cb
#define PI 3.14159
int API = 100;  // 期待: API  実際: API (OK - 完全一致のみ展開)
```

## 根本原因

現在の実装は**テキストベース**でマクロ展開を行っているため：
- 文字列リテラルとコードの区別ができない
- コメント内のマクロも展開される可能性がある
- エスケープシーケンスを考慮していない

## 解決策：トークンベースのプリプロセス

### アーキテクチャ変更

#### 現在のフロー
```
Source Code (Text)
  ↓
Preprocessor (Text → Text)  ← 文字列とコードを区別できない
  ↓
Lexer (Text → Tokens)
  ↓
Parser (Tokens → AST)
```

#### 新しいフロー
```
Source Code (Text)
  ↓
Lexer (Text → Tokens)  ← 文字列リテラルをTOKEN_STRINGとして識別
  ↓
Preprocessor (Tokens → Tokens)  ← TOKEN_IDENTIFIERのみを展開
  ↓
Parser (Tokens → AST)
```

### 実装計画

#### Phase 1: レキサーの拡張
1. `Token` 構造体にプリプロセッサディレクティブ用のトークンタイプを追加
   - `TOKEN_PREPROCESSOR_DEFINE`
   - `TOKEN_PREPROCESSOR_UNDEF`
   - `TOKEN_HASH` (#)

2. レキサーが `#define` を検出したら、行全体を特別なトークンとして返す

#### Phase 2: トークンベースプリプロセッサの実装
1. `TokenPreprocessor` クラスを作成
   ```cpp
   class TokenPreprocessor {
   public:
       std::vector<Token> process(const std::vector<Token>& tokens);
   private:
       void processDefine(const Token& directiveToken);
       Token expandMacro(const Token& identToken);
       bool isStringLiteral(const Token& token);
   };
   ```

2. トークン列をスキャンして：
   - `TOKEN_PREPROCESSOR_DEFINE` → マクロ定義を登録
   - `TOKEN_IDENTIFIER` → マクロ展開を試みる
   - `TOKEN_STRING` → スキップ（展開しない）
   - `TOKEN_COMMENT` → スキップ

#### Phase 3: main.cpp の変更
```cpp
// 新しいフロー
Lexer lexer(source);
auto tokens = lexer.tokenize();

TokenPreprocessor preprocessor;
auto processedTokens = preprocessor.process(tokens);

Parser parser(processedTokens);
auto ast = parser.parse();
```

## メリット

1. **文字列リテラルを自動的にスキップ** - レキサーがすでに識別済み
2. **トークン境界が明確** - 識別子の部分一致を完全に防げる
3. **コメントも自動的にスキップ** - レキサーがコメントを除去
4. **標準的なアプローチ** - C/C++の実装と同じ方針

## デメリット

1. **アーキテクチャの大きな変更** - レキサーとプリプロセッサの統合が必要
2. **既存コードの書き換え** - `macro_expander.cpp` の大部分を書き直し
3. **テストの更新** - 既存の24テストを調整

## 代替案：テキストベースの改善

文字列リテラルとコメントをスキップする処理を追加：

```cpp
std::string MacroExpander::expandAll(const std::string& text) {
    std::string result;
    size_t i = 0;
    bool inString = false;
    bool inComment = false;
    
    while (i < text.length()) {
        // 文字列リテラルの開始/終了を検出
        if (text[i] == '"' && (i == 0 || text[i-1] != '\\')) {
            inString = !inString;
            result += text[i++];
            continue;
        }
        
        // コメントの検出
        if (!inString && i + 1 < text.length() && text[i] == '/' && text[i+1] == '/') {
            inComment = true;
        }
        
        if (inComment && text[i] == '\n') {
            inComment = false;
        }
        
        // 文字列/コメント内ではマクロ展開しない
        if (inString || inComment) {
            result += text[i++];
            continue;
        }
        
        // 通常のマクロ展開処理
        // ...
    }
    
    return result;
}
```

**代替案の問題点：**
- エスケープシーケンス処理が複雑（`\"`, `\\` など）
- 生文字列リテラルの対応が必要
- マルチラインコメントの対応
- 依然として識別子境界の判定が難しい

## 推奨：トークンベースアプローチ

理由：
1. より堅牢で保守しやすい
2. C/C++の標準的なアプローチ
3. 将来的な拡張（条件コンパイルなど）が容易
4. レキサーがすでに複雑な解析を行っている

## 実装ステップ

1. **Phase 1: レキサーの拡張**（1-2日）
   - プリプロセッサディレクティブ用トークンタイプを追加
   - `#define`, `#undef` を検出してトークン化

2. **Phase 2: TokenPreprocessor実装**（2-3日）
   - トークン列を受け取るプリプロセッサクラス
   - TOKEN_IDENTIFIERのみをマクロ展開

3. **Phase 3: 統合とテスト**（1-2日）
   - main.cppの変更
   - 既存テストの更新
   - 新しいテストケースの追加

## テストケース

```cb
// テスト1: 文字列リテラル内は展開しない
#define PI 3.14159
println("PI = ");  // → println("PI = ");

// テスト2: コメント内は展開しない
#define MAX 100
// MAX should not be expanded in comments

// テスト3: 識別子の境界
#define PI 3.14159
int API = 100;  // → int API = 100;

// テスト4: エスケープシーケンス
println("Quote: \"PI\"");  // → println("Quote: \"PI\"");
```

## 次のアクション

ユーザーの決定を待つ：
1. **トークンベースアプローチを実装**（推奨）
2. テキストベースの改善で対応
3. 現状維持（Phase 2で対応）
