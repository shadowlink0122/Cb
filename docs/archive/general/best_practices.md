# Cb言語開発ベストプラクティス（v0.9.1）

## 📚 概要

このドキュメントは、Cb言語プロジェクトでの開発におけるベストプラクティスをまとめたものです。v0.9.1のリファクタリング（Phase 1-4）で学んだ教訓を基に作成されています。

---

## 🏗️ アーキテクチャ設計

### 1. 単一責任の原則（SRP）

**原則**: 各クラス・ファイルは1つの明確な責任を持つべき

**実践例**:
```cpp
// ✅ 良い例: 責任が明確
class ExpressionParser {
    // 式の解析のみに集中
    ASTNode* parseExpression();
    ASTNode* parseAssignment();
    // ...
};

class StatementParser {
    // 文の解析のみに集中
    ASTNode* parseStatement();
    ASTNode* parseIfStatement();
    // ...
};

// ❌ 悪い例: 責任が混在
class Parser {
    // 全ての解析が混在（5000行超）
    ASTNode* parseExpression();
    ASTNode* parseStatement();
    ASTNode* parseType();
    ASTNode* parseStruct();
    // ...
};
```

**v0.9.1での適用**:
- RecursiveParserを5つのパーサークラスに分割
- 各クラスが明確な責任を持つ（式、文、宣言、型、構造体）

### 2. 1000行ルール

**原則**: 1ファイルは1000行を超えないようにする

**理由**:
- 可読性の向上
- メンテナンスの容易さ
- レビューのしやすさ
- バグの発見が容易

**実践**:
```bash
# ファイルサイズチェック
wc -l src/**/*.cpp

# 1000行を超えるファイルを警告
find src -name "*.cpp" -exec wc -l {} + | awk '$1 > 1000 {print}'
```

**v0.9.1での状況**:
- Phase 4完了時点: recursive_parser.cpp 5606行（Phase 5で削減予定）
- 各パーサーファイル: 200-400行（目標達成）

### 3. friend宣言の適切な使用

**原則**: friend宣言は過渡期のみ使用、最終的には削減

**v0.9.1での使用**:
```cpp
// Phase 2-4: 段階的リファクタリングのため使用
class RecursiveParser {
    friend class ExpressionParser;
    friend class StatementParser;
    // ...
private:
    Token current_token_;  // friendクラスからアクセス可能
};
```

**Phase 5以降の計画**:
- 実装移行完了後、friend宣言を削減
- 必要な内部状態はpublicメソッド経由でアクセス
- カプセル化の強化

---

## 📝 コーディング規約

### 1. コメント規約

#### Doxygenスタイルの使用

**必須要素**:
- `@brief`: メソッドの概要（1行）
- `@param`: パラメータの説明
- `@return`: 戻り値の説明

**推奨要素**:
- 詳細な説明
- サポートする構文の列挙
- 使用例
- 注意事項

**実践例**:
```cpp
/**
 * @brief 式を解析してASTノードを返す
 * @return 解析された式のASTノード
 * 
 * サポートする式:
 * - 代入式: a = 10
 * - 複合代入: a += 5
 * - 三項演算子: x > 0 ? 1 : -1
 * - 二項演算子: a + b
 * - 単項演算子: -a, !flag
 * 
 * 使用例:
 * ```cpp
 * ASTNode* expr = parseExpression();
 * ```
 * 
 * 注意: カンマ演算子は低優先度
 */
ASTNode* ExpressionParser::parseExpression() {
    // 実装
}
```

#### ファイルヘッダーコメント

**必須**:
```cpp
// [ファイル名] - [役割の簡潔な説明]
// [英語名] - [英語での説明]
//
// このファイルは、[詳細な説明]
//
// 【サポートする機能】:
// 1. [機能1]
// 2. [機能2]
// ...
```

**v0.9.1での例**:
```cpp
// Expression Parser - 式解析を担当
// Phase 2-3: RecursiveParserへの委譲実装 + ドキュメント化
//
// このファイルは、Cb言語の式解析を担当します。
//
// 【演算子優先順位】（14レベル）:
// Level 1: 代入演算子 (=, +=, -=, ...)
// Level 2: 三項演算子 (? :)
// ...
```

### 2. 命名規約

#### クラス名
- **形式**: PascalCase
- **例**: `ExpressionParser`, `StatementParser`, `TypeParser`

#### メソッド名
- **形式**: camelCase
- **例**: `parseExpression()`, `parseIfStatement()`, `isValidType()`

#### 変数名
- **形式**: snake_case
- **例**: `current_token_`, `typedef_map_`, `struct_definitions_`

#### 定数名
- **形式**: UPPER_SNAKE_CASE
- **例**: `MAX_DEPTH`, `DEFAULT_SIZE`

### 3. コードフォーマット

**インデント**: 4スペース

**中括弧**:
```cpp
// ✅ 良い例: K&Rスタイル
if (condition) {
    // code
}

// ❌ 悪い例: Allmanスタイル（非推奨）
if (condition)
{
    // code
}
```

**行の長さ**: 最大100文字（推奨80文字）

---

## 🧪 テスト駆動開発

### 1. テストファースト

**原則**: 実装前にテストを書く

**プロセス**:
1. テストケースを作成
2. テストが失敗することを確認（Red）
3. 最小限の実装でテストを通す（Green）
4. リファクタリング（Refactor）

### 2. テストカバレッジ

**目標**: 全機能を網羅的にテスト

**v0.9.1の状況**:
- 統合テスト: **2380個**
- 単体テスト: **30個**
- カバレッジ: **全機能を網羅**

**テストの種類**:
```
tests/
├── cases/           # 統合テスト（機能別）
│   ├── arithmetic/  # 算術演算
│   ├── array/       # 配列
│   ├── struct/      # 構造体
│   ├── pointer/     # ポインタ
│   └── ...
├── integration/     # 統合テスト実行
└── unit/            # 単体テスト
```

### 3. テスト実行頻度

**推奨**:
- コミット前: 必須
- 実装変更後: 必須
- リファクタリング中: 各ステップで必須

**コマンド**:
```bash
# フルテスト
make clean && make -j4 && make integration-test

# クイックテスト（ビルドのみ）
make -j4

# 特定のテストのみ
./main tests/cases/arithmetic/basic.cb
```

---

## 🔄 リファクタリング戦略

### 1. 段階的アプローチ

**原則**: 大規模な変更は小さなフェーズに分割

**v0.9.1の例**:
- **Phase 1**: 基盤構築（ディレクトリ、ヘッダー）
- **Phase 2**: 委譲実装（既存実装を保持）
- **Phase 3**: ドキュメント化（コード品質向上）
- **Phase 4**: ドキュメント整備（知識の体系化）
- **Phase 5**: 実装移行（実際の分離）← 予定

**利点**:
- 各フェーズで明確な成果
- 問題の早期発見
- ロールバックが容易

### 2. テスト駆動リファクタリング

**プロセス**:
1. 既存のテストを全て合格させる
2. リファクタリング実施
3. テストを再実行（全合格を確認）
4. コミット

**v0.9.1での実践**:
```bash
# Phase 2完了時
Total:  2380
Passed: 2380  ← 100%合格
Failed: 0

# Phase 3完了時
Total:  2380
Passed: 2380  ← 100%合格維持
Failed: 0
```

### 3. パフォーマンスモニタリング

**原則**: リファクタリング前後でパフォーマンスを計測

**許容範囲**: ±10%以内

**v0.9.1での推移**:
```
Baseline (v0.9.0): 830ms
Phase 1: 833ms (+0.3%)  ← 許容範囲
Phase 2: 863ms (+3.9%)  ← 許容範囲
Phase 3: 804ms (-3.1%)  ← 改善！🎉
```

**計測方法**:
```bash
# 統合テストの実行時間を記録
make integration-test 2>&1 | grep "Total time"
```

---

## 📦 バージョン管理

### 1. コミットメッセージ規約

**形式**:
```
<type>: <subject>

<body>

<footer>
```

**type の種類**:
- `feat`: 新機能
- `fix`: バグ修正
- `docs`: ドキュメントのみの変更
- `refactor`: リファクタリング
- `test`: テストの追加・修正
- `chore`: ビルドプロセス等の変更

**v0.9.1での例**:
```
Phase 3完了: 全パーサーのドキュメント化とパフォーマンス改善

- ExpressionParser: 演算子優先順位の詳細説明（14レベル）
- StatementParser: 制御構造とジャンプ文の説明
- DeclarationParser: 変数・関数・typedef宣言の説明
- TypeParser: 型解析と検証の説明
- StructParser: 構造体・Union・Enumの説明

Doxygenスタイル:
- @brief: メソッドの概要
- @param: パラメータの説明
- @return: 戻り値の説明
- サポート構文の列挙
- 使用例の追加

パフォーマンス:
- 1031ms → 804ms（22%改善）
- 全2380テスト合格維持
```

### 2. ブランチ戦略

**推奨**: Git Flow

**ブランチの種類**:
- `main`: 本番環境用（安定版のみ）
- `develop`: 開発用（次バージョン）
- `feature/*`: 新機能開発用
- `hotfix/*`: 緊急バグ修正用

**v0.9.1での使用**:
```
feature/pointer2  ← 現在のブランチ
├─ Phase 1-4の作業
└─ Phase 5の作業（予定）
```

### 3. タグ付け

**形式**: `vX.Y.Z`（セマンティックバージョニング）

**例**:
```bash
# v0.9.1のタグ付け
git tag -a v0.9.1 -m "v0.9.1: リファクタリング Phase 1-4 完了"
git push origin v0.9.1
```

---

## 🐛 デバッグ戦略

### 1. ログ活用

**推奨**: デバッグモードの実装

**v0.9.1での実装**:
```cpp
class RecursiveParser {
private:
    bool debug_mode_;  // デバッグフラグ
    
public:
    void setDebugMode(bool debug) { debug_mode_ = debug; }
};
```

**使用例**:
```bash
# デバッグモードで実行
./main --debug test.cb
```

### 2. エラーメッセージ

**原則**: 詳細で有用なエラーメッセージを提供

**良い例**:
```
Error at line 10, column 5:
    int x = ;
            ^
Expected expression after '='
```

**悪い例**:
```
Parse error
```

### 3. アサーション

**推奨**: 内部状態の検証にアサーションを使用

**例**:
```cpp
void processToken() {
    assert(current_token_.type != TOKEN_EOF && "Unexpected EOF");
    // 処理
}
```

---

## 📈 継続的改善

### 1. コードレビュー

**チェックポイント**:
- [ ] 単一責任の原則を守っているか
- [ ] 命名規約に従っているか
- [ ] コメントが適切に書かれているか
- [ ] テストが追加されているか
- [ ] パフォーマンスへの影響は許容範囲か

### 2. 定期的なリファクタリング

**推奨頻度**: 2-3ヶ月ごと

**チェック項目**:
- ファイルサイズ（1000行ルール）
- コードの重複
- 循環依存
- 未使用のコード

### 3. ドキュメントの更新

**原則**: コード変更時にドキュメントも更新

**必須ドキュメント**:
- README.md
- release_notes/vX.Y.Z.md
- docs/refactoring_progress.md（リファクタリング時）
- 各ファイルのヘッダーコメント

---

## 🎯 v0.9.1で学んだ教訓

### 1. ドキュメント化がパフォーマンスを改善

**発見**: コードの整理とドキュメント化により、パフォーマンスも向上

**結果**: Phase 3で22%のパフォーマンス改善（863ms → 804ms）

**理由**:
- 構造が明確になり、非効率な箇所が可視化される
- コードレビュー時に最適化の余地を発見しやすい
- 不要なコードが明確になり、削除できる

### 2. 段階的アプローチの重要性

**学び**: 大規模リファクタリングは、小さなフェーズに分割することで成功率が高まる

**v0.9.1の成功要因**:
- Phase 1: 基盤構築（構造のみ）
- Phase 2: 委譲実装（破壊的変更なし）
- Phase 3: ドキュメント化（品質向上）
- Phase 4: ドキュメント整備（知識体系化）

各フェーズで明確な成果を得て、テスト100%合格を維持

### 3. friend宣言の有効活用

**学び**: friend宣言により、段階的リファクタリングが可能

**利点**:
- 既存実装を保持しながら構造改善
- テストの100%合格を保証
- Phase 5で徐々に削減可能

**注意**: 過度な使用は避け、過渡期のみ使用

### 4. テストの重要性

**学び**: 全2380テストの100%合格を維持することで、信頼性を保証

**効果**:
- リファクタリング時の安心感
- デグレードの即座の検出
- 顧客への信頼提供

---

## 📚 参考リソース

### プロジェクト内ドキュメント
- `docs/refactoring_progress.md` - 進捗レポート
- `docs/architecture.md` - アーキテクチャ図
- `docs/phase5_guide.md` - Phase 5実装ガイド
- `release_notes/v0.9.1.md` - リリースノート

### 外部リソース
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [Doxygen Manual](https://www.doxygen.nl/manual/)
- [Git Flow](https://nvie.com/posts/a-successful-git-branching-model/)
- [Semantic Versioning](https://semver.org/)

---

**作成日**: 2025年1月  
**バージョン**: v0.9.1  
**最終更新**: Phase 4完了時
