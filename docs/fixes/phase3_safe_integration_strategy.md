# Phase 3: 安全な統合戦略

## 問題の分析

RecursiveParser を大幅に変更すると、既存の2968個のテストに影響を与えるリスクが高い。

## 新しいアプローチ: ハイブリッド方式

TokenPreprocessor でトークン列を処理し、その結果を文字列に戻してから RecursiveParser に渡す。

### メリット
1. RecursiveParser を変更しない（リスクゼロ）
2. 既存のテストが全て動作する
3. 文字列リテラルバグは修正される

### デメリット
1. トークン→文字列の変換オーバーヘッド（最小限）
2. 理想的なアーキテクチャではない

## 実装計画

```cpp
// main.cpp の変更

// 1. ソースを読み込み
std::string source = ...;

// 2. レキサーでトークン化
RecursiveLexer lexer(source);
std::vector<Token> tokens = lexer.tokenizeAll();

// 3. TokenPreprocessor で処理
TokenPreprocessor preprocessor;
std::vector<Token> processedTokens = preprocessor.process(tokens);

// 4. トークン列を文字列に戻す
std::string processedSource = tokensToString(processedTokens);

// 5. RecursiveParser に渡す（変更なし）
RecursiveParser parser(processedSource, filename);
ASTNode* root = parser.parseProgram();
```

### tokensToString() 関数

```cpp
std::string tokensToString(const std::vector<Token>& tokens) {
    std::string result;
    for (const auto& token : tokens) {
        if (!result.empty() && needsSpace(lastToken, token)) {
            result += " ";
        }
        result += token.value;
    }
    return result;
}
```

## Phase 4 での改善

将来的に RecursiveParser をトークン列ベースに完全移行する際の準備：

1. RecursiveParser のリファクタリング
2. トークンストリームの直接処理
3. パフォーマンス最適化

しかし、現時点では安全性を優先する。
