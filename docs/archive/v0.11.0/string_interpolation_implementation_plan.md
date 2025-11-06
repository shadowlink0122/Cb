# 文字列補間 実装計画

## プロジェクト概要

**機能**: 文字列補間 (String Interpolation)  
**開始日**: 2025年10月27日  
**目標バージョン**: v0.11.0  
**優先度**: 高

## 実装フェーズ

### Phase 1: 字句解析 (Lexer) - 1-2日

#### タスク
- [ ] 新しいトークン型の定義
  - [ ] `TOKEN_STRING_INTERPOLATION_START` (`${`)
  - [ ] `TOKEN_STRING_INTERPOLATION_END` (`}` in string context)
  - [ ] `TOKEN_STRING_PART` (補間間の文字列)

- [ ] 字句解析ロジックの実装
  - [ ] 文字列モードの状態管理
  - [ ] `${`検出とトークン分割
  - [ ] エスケープ処理 (`\\${`)
  - [ ] ネストした括弧のカウント（式内の`{}`を正しく処理）

- [ ] テスト
  - [ ] 単純な補間のトークン化
  - [ ] 複数補間のトークン化
  - [ ] エスケープのテスト
  - [ ] エラーケース（閉じ括弧なし）

#### 実装ファイル
```
src/common/token.h              # トークン型定義
src/frontend/lexer/lexer.h      # Lexerクラス拡張
src/frontend/lexer/lexer.cpp    # メインロジック
```

#### テストファイル
```
tests/unit/lexer/test_string_interpolation_lexer.cpp
```

### Phase 2: 構文解析 (Parser) - 2-3日

#### タスク
- [ ] AST ノードの定義
  - [ ] `StringInterpolationNode`
  - [ ] `StringPart` (文字列部分)
  - [ ] `InterpolationExpression` (補間式)

- [ ] パーサーロジック
  - [ ] `parse_string_literal()` の拡張
  - [ ] 補間式のパース
  - [ ] 複数部分の収集
  - [ ] エラーハンドリング

- [ ] テスト
  - [ ] 基本的な補間のパース
  - [ ] 複雑な式のパース
  - [ ] ネストした式
  - [ ] エラーケース

#### 実装ファイル
```
src/common/ast.h                                     # AST定義
src/frontend/parser/expressions/primary.cpp          # パーサー拡張
```

#### テストファイル
```
tests/unit/parser/test_string_interpolation_parser.cpp
```

### Phase 3: 評価器 (Evaluator) - 2-3日

#### タスク
- [ ] 評価ロジックの実装
  - [ ] `evaluate_string_interpolation()`
  - [ ] 部分ごとの評価とマージ
  - [ ] 効率的な文字列構築

- [ ] 型変換システム
  - [ ] `value_to_string()` 実装
  - [ ] 各型の変換ロジック
    - [ ] 整数型 (int, long, short, tiny)
    - [ ] 符号なし整数型
    - [ ] 文字型 (char)
    - [ ] 文字列型 (string)
    - [ ] ブール型 (bool)
    - [ ] ポインタ型
    - [ ] 浮動小数点型 (float, double) - 将来

- [ ] テスト
  - [ ] 各型の変換テスト
  - [ ] 式評価テスト
  - [ ] 複数補間テスト

#### 実装ファイル
```
src/backend/interpreter/evaluator/primary_evaluator.cpp   # 評価ロジック
src/backend/interpreter/utils/value_converter.h           # 型変換ヘッダー
src/backend/interpreter/utils/value_converter.cpp         # 型変換実装
```

#### テストファイル
```
tests/cases/string_interpolation/test_basic.cb
tests/cases/string_interpolation/test_types.cb
```

### Phase 4: エラーハンドリング - 1-2日

#### タスク
- [ ] コンパイルエラー
  - [ ] 閉じ括弧なし
  - [ ] 不正な式
  - [ ] 未定義変数

- [ ] ランタイムエラー
  - [ ] 変換不可能な型
  - [ ] Nullポインタ参照

- [ ] エラーメッセージの改善
  - [ ] 行番号・列番号の表示
  - [ ] 具体的な修正提案

- [ ] テスト
  - [ ] 各エラーケースの検証
  - [ ] エラーメッセージの確認

#### 実装ファイル
```
src/common/error.h                  # エラー定義
src/common/error.cpp                # エラー処理
```

#### テストファイル
```
tests/cases/string_interpolation/test_errors.cb
```

### Phase 5: 統合テスト - 2-3日

#### タスク
- [ ] 包括的テストスイート作成
  - [ ] 基本機能（10テスト）
  - [ ] 型変換（10テスト）
  - [ ] 複雑な式（10テスト）
  - [ ] メンバー・配列アクセス（10テスト）
  - [ ] エッジケース（10テスト）
  - [ ] エラーケース（10テスト）

- [ ] 統合テストフレームワーク組み込み
  - [ ] `test_string_interpolation.hpp` 作成
  - [ ] Makefileへの追加

- [ ] 実行とデバッグ
  - [ ] 全テスト実行
  - [ ] バグ修正
  - [ ] リグレッションテスト

#### テストファイル
```
tests/cases/string_interpolation/
  ├── test_basic.cb                      # 基本
  ├── test_variables.cb                  # 変数参照
  ├── test_types.cb                      # 型変換
  ├── test_expressions.cb                # 式評価
  ├── test_member_access.cb              # メンバーアクセス
  ├── test_array_access.cb               # 配列アクセス
  ├── test_multiple.cb                   # 複数補間
  ├── test_escape.cb                     # エスケープ
  ├── test_edge_cases.cb                 # エッジケース
  └── test_errors.cb                     # エラー

tests/integration/string_interpolation/
  └── test_string_interpolation.hpp
```

### Phase 6: 最適化 - 1-2日

#### タスク
- [ ] パフォーマンス測定
  - [ ] ベンチマークテスト作成
  - [ ] 補間 vs 連結の比較

- [ ] 最適化実装
  - [ ] 文字列サイズ事前予約
  - [ ] 定数畳み込み
  - [ ] 頻出変換のキャッシング（オプション）

- [ ] 最適化検証
  - [ ] パフォーマンステスト再実行
  - [ ] メモリ使用量チェック

#### 実装ファイル
```
src/backend/optimizer/string_interpolation_optimizer.h
src/backend/optimizer/string_interpolation_optimizer.cpp
```

#### テストファイル
```
tests/performance/string_interpolation_benchmark.cb
```

### Phase 7: ドキュメント - 1日

#### タスク
- [ ] 実装レポート作成
  - [ ] 実装の詳細
  - [ ] 技術的決定の理由
  - [ ] 既知の制限事項

- [ ] 仕様書への追加
  - [ ] `docs/spec.md` 更新
  - [ ] 構文セクション追加
  - [ ] 使用例追加

- [ ] チュートリアル作成
  - [ ] 基本的な使い方
  - [ ] 実践例
  - [ ] よくある間違い

- [ ] リリースノート
  - [ ] v0.11.0 の主要機能として記載

#### ドキュメントファイル
```
docs/features/string_interpolation_implementation.md
docs/spec.md (更新)
docs/tutorial/string_interpolation_guide.md
release_notes/v0.11.0.md
```

## タイムライン（ガントチャート風）

```
Week 1 (Day 1-7):
  Day 1-2:  Phase 1 (Lexer)               [████████░░░░░░]
  Day 3-5:  Phase 2 (Parser)              [░░░░░░░░████████]
  Day 6-7:  Phase 3 (Evaluator) 開始     [░░░░░░░░░░░░██░░]

Week 2 (Day 8-14):
  Day 8-9:  Phase 3 (Evaluator) 完了     [██████░░░░░░░░░░]
  Day 10-11: Phase 4 (Error Handling)    [░░░░░░████████░░]
  Day 12-14: Phase 5 (Integration Test)  [░░░░░░░░░░████░░]

Week 3 (Day 15-16):
  Day 15:   Phase 6 (Optimization)       [██████████░░░░░░]
  Day 16:   Phase 7 (Documentation)      [░░░░░░░░░░██████]
```

**総計**: 約16日（2-3週間）

## チェックポイント

### Checkpoint 1: Phase 1 完了後
- [ ] 字句解析が正しく動作
- [ ] トークン分割が正確
- [ ] エスケープ処理が正しい
- [ ] 基本的なテストが通過

**判断基準**: 10個以上の字句解析テストが全て成功

### Checkpoint 2: Phase 3 完了後
- [ ] 基本的な補間が動作
- [ ] 型変換が正しい
- [ ] 式評価が正確
- [ ] エンドツーエンドで動作

**判断基準**: 実際の`.cb`ファイルで補間が動作

### Checkpoint 3: Phase 5 完了後
- [ ] 50個以上のテストが成功
- [ ] エッジケースがカバーされている
- [ ] エラーハンドリングが適切
- [ ] 統合テストフレームワークに組み込み

**判断基準**: テストスイート成功率 100%

## リスク管理

### リスク1: 字句解析の複雑化
**対策**:
- 段階的実装（まず単純なケースから）
- 詳細なユニットテスト
- デバッグ用ログの充実

### リスク2: パフォーマンス問題
**対策**:
- 早期のベンチマーク測定
- プロファイリングツール使用
- 必要に応じて最適化

### リスク3: エッジケースのバグ
**対策**:
- 包括的テストスイート
- コードレビュー
- 段階的なリリース

## 成功基準

### 必須基準（Phase 5完了時）
- ✅ 基本的な変数補間が動作
- ✅ 全ての基本型が変換可能
- ✅ 式評価が正確
- ✅ 50個以上のテストが成功
- ✅ エラーメッセージが明確

### 推奨基準（Phase 6-7完了時）
- ✅ パフォーマンスが文字列連結と同等以上
- ✅ 包括的なドキュメント整備
- ✅ 統合テストフレームワークに組み込み
- ✅ リリースノート作成

## 次のアクション

1. **即座**: Phase 1（字句解析）の実装開始
2. **次**: トークン定義の追加
3. **その後**: 字句解析ロジックの実装

---

**関連ドキュメント**:
- 設計: `docs/features/string_interpolation.md`
- 実装レポート: `docs/features/string_interpolation_implementation.md`（実装後）
- テストケース: `tests/cases/string_interpolation/`
