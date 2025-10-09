# 段階的リファクタリング計画（実践的アプローチ）

## フェーズ1: DRY原則の適用 - 重複コード削除

### 1.1 Parser内の重複パターン
- 二項演算子パーサーの共通化
- エラー処理の統一
- トークン消費パターンの統一

### 1.2 Evaluator内の重複パターン  
- 型変換処理の統一
- エラー処理の統一
- 値取得パターンの統一

### 1.3 Variable Manager内の重複
- 変数検索の統一
- スコープ処理の統一

## フェーズ2: ヘルパー関数の抽出

### 2.1 Parser Utils
- 共通のトークンチェック関数
- 型解析ヘルパー
- エラーメッセージ生成

### 2.2 Evaluator Utils
- 型変換ヘルパー
- 数値演算ヘルパー
- ポインタ演算ヘルパー

## フェーズ3: パフォーマンス最適化

### 3.1 不要なコピーの削減
- const参照の活用
- ムーブセマンティクスの活用

### 3.2 アルゴリズム改善
- 検索の最適化
- キャッシュの活用

## 実施手順

1. **重複コードの特定** (今回)
2. **共通関数の抽出** (小さな変更)
3. **テスト** (各ステップで)
4. **コミット** (動作確認後)
5. **繰り返し**

## 今回の具体的な作業

### ステップ1: Parser の二項演算子の共通化

現在の問題:
```cpp
ASTNode* RecursiveParser::parseLogicalOr() {
    // ほぼ同じコード
}
ASTNode* RecursiveParser::parseLogicalAnd() {
    // ほぼ同じコード  
}
ASTNode* RecursiveParser::parseBitwiseOr() {
    // ほぼ同じコード
}
// ... 8個の類似関数
```

解決策:
```cpp
ASTNode* parseBinaryOperation(
    ASTNode* (RecursiveParser::*next_level)(),
    std::vector<TokenType> operators,
    ASTNodeType node_type
);
```

期待される効果:
- コード削減: ~400行
- 保守性向上
- バグ修正の容易化
