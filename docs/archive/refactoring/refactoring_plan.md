# リファクタリング計画書（改訂版）

実施日: 2025年10月6日  
目的: コードの保守性向上、段階的な改善、テスト維持

## 現実的なアプローチ

大規模な一括リファクタリングではなく、**段階的で安全な改善**を実施します。

## フェーズ1: 即座に実施可能な改善

### 現状の問題
- `recursive_parser.cpp` が5588行で単一責任原則違反
- 複数の異なる構文解析が混在
- デバッグとメンテナンスが困難

### 分割計画

```
src/frontend/recursive_parser/
├── recursive_parser.cpp (メインパーサー: ~800行)
├── recursive_lexer.cpp (既存)
├── parsers/
│   ├── expression_parser.cpp    # 式の解析 (~800行)
│   ├── statement_parser.cpp     # 文の解析 (~700行)
│   ├── declaration_parser.cpp   # 宣言の解析 (~600行)
│   ├── type_parser.cpp          # 型の解析 (~400行)
│   ├── struct_parser.cpp        # 構造体の解析 (~500行)
│   ├── function_parser.cpp      # 関数の解析 (~600行)
│   ├── interface_parser.cpp     # インターフェースの解析 (~400行)
│   ├── enum_parser.cpp          # 列挙型の解析 (~300行)
│   └── typedef_parser.cpp       # typedef の解析 (~400行)
└── utils/
    ├── parser_utils.cpp         # 共通ユーティリティ
    └── type_resolver.cpp        # 型解決ユーティリティ
```

## フェーズ2: Evaluator のリファクタリング (5869行 → 複数ファイル)

### 分割計画

```
src/backend/interpreter/evaluator/
├── expression_evaluator.cpp (メイン: ~800行)
├── evaluators/
│   ├── arithmetic_evaluator.cpp    # 算術演算 (~400行)
│   ├── comparison_evaluator.cpp    # 比較演算 (~300行)
│   ├── logical_evaluator.cpp       # 論理演算 (~300行)
│   ├── bitwise_evaluator.cpp       # ビット演算 (~300行)
│   ├── assignment_evaluator.cpp    # 代入演算 (~400行)
│   ├── call_evaluator.cpp          # 関数呼び出し (~600行)
│   ├── member_evaluator.cpp        # メンバーアクセス (~500行)
│   ├── array_evaluator.cpp         # 配列アクセス (~400行)
│   ├── pointer_evaluator.cpp       # ポインタ演算 (~500行)
│   └── cast_evaluator.cpp          # 型キャスト (~300行)
```

## フェーズ3: Interpreter のリファクタリング (5896行 → 複数ファイル)

### 分割計画

```
src/backend/interpreter/core/
├── interpreter.cpp (メイン: ~1000行)
├── executors/
│   ├── control_flow_executor.cpp   # if/while/for (~600行)
│   ├── function_executor.cpp       # 関数実行 (~800行)
│   ├── struct_executor.cpp         # 構造体操作 (~600行)
│   ├── array_executor.cpp          # 配列操作 (~500行)
│   └── module_executor.cpp         # モジュール処理 (~400行)
```

## フェーズ4: Variable Manager のリファクタリング (4057行 → 複数ファイル)

### 分割計画

```
src/backend/interpreter/managers/
├── variable_manager.cpp (メイン: ~800行)
├── variable/
│   ├── variable_storage.cpp        # 変数ストレージ (~600行)
│   ├── variable_lookup.cpp         # 変数検索 (~400行)
│   ├── variable_assignment.cpp     # 変数代入 (~500行)
│   ├── struct_variable_manager.cpp # 構造体変数 (~800行)
│   ├── array_variable_manager.cpp  # 配列変数 (~600行)
│   └── scope_manager.cpp           # スコープ管理 (~400行)
```

## フェーズ5: DRY原則の適用

### 重複コード削除
- 共通パターンの抽出
- ヘルパー関数の統合
- テンプレート化可能な処理

### パフォーマンス最適化
- 不要なコピーの削減
- キャッシュの活用
- アルゴリズムの改善

## 実施順序

1. **Phase 1a**: Parser の準備
   - 新しいディレクトリ構造作成
   - 共通インターフェース定義

2. **Phase 1b**: Parser の分割実施
   - 1ファイルずつ分割
   - 各ステップでテスト実行

3. **Phase 2**: Evaluator の分割
4. **Phase 3**: Interpreter の分割
5. **Phase 4**: Variable Manager の分割
6. **Phase 5**: DRY原則適用とパフォーマンス最適化

## 成功基準

- ✅ 全テスト (2383個) が100%パス
- ✅ 各ファイルが1000行以下
- ✅ 単一責任原則の遵守
- ✅ ビルド時間の維持または改善
- ✅ 実行時パフォーマンスの維持または改善

## リスク管理

- 各変更後に必ずテスト実行
- Git コミットを細かく実施
- 問題発生時は即座にロールバック
- 段階的な実施で影響範囲を限定
