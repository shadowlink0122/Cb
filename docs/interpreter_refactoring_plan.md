# インタプリタリファクタリング計画

**作成日**: 2025年10月7日  
**対象**: `src/backend/interpreter/` ディレクトリ  
**目標**: 人間がメンテナンスしやすい粒度でファイル・フォルダ・関数を分割  
**目安**: 1ファイル1000行以内（技術的に困難な場合を除く）

---

## 1. 現状分析

### 1.1 ファイル行数の現状

| ファイル | 行数 | 状態 | 優先度 |
|---------|------|------|--------|
| **interpreter.cpp** | **5,896行** | ❌ 超過 (5.9倍) | **最優先** |
| **expression_evaluator.cpp** | **5,869行** | ❌ 超過 (5.9倍) | **最優先** |
| **variable_manager.cpp** | **4,057行** | ❌ 超過 (4.1倍) | **高** |
| **statement_executor.cpp** | **2,722行** | ❌ 超過 (2.7倍) | **高** |
| **array_manager.cpp** | **1,912行** | ❌ 超過 (1.9倍) | **中** |
| **output_manager.cpp** | **1,371行** | ❌ 超過 (1.4倍) | **中** |
| type_inference.cpp | 722行 | ✅ OK | 低 |
| type_manager.cpp | 516行 | ✅ OK | - |
| common_operations.cpp | 518行 | ✅ OK | - |
| array_processing_service.cpp | 388行 | ✅ OK | - |
| pointer_metadata.cpp | 334行 | ✅ OK | - |
| error_handler.cpp | 315行 | ✅ OK | - |

**総行数**: 27,519行  
**問題ファイル**: 6個（合計22,827行、全体の83%）

### 1.2 ディレクトリ構造

```
src/backend/interpreter/
├── core/              # コアロジック
│   ├── interpreter.cpp/h       (5,896行) ← 最大の問題
│   ├── type_inference.cpp/h    (722行)
│   ├── error_handler.cpp/h     (315行)
│   └── pointer_metadata.cpp/h  (334行)
├── evaluator/         # 式評価
│   └── expression_evaluator.cpp/h (5,869行) ← 2番目の問題
├── executor/          # 文実行
│   └── statement_executor.cpp/h (2,722行) ← 3番目の問題
├── managers/          # データ管理
│   ├── variable_manager.cpp/h  (4,057行) ← 4番目の問題
│   ├── array_manager.cpp/h     (1,912行)
│   ├── type_manager.cpp/h      (516行)
│   ├── enum_manager.cpp/h      (104行)
│   └── common_operations.cpp/h (518行)
├── services/          # ヘルパーサービス
│   ├── expression_service.cpp/h (150行)
│   ├── variable_access_service.cpp/h (185行)
│   ├── debug_service.cpp/h (165行)
│   └── array_processing_service.cpp/h (388行)
└── output/            # 出力管理
    └── output_manager.cpp/h (1,371行)
```

### 1.3 問題点

#### ❌ **interpreter.cpp (5,896行、82メソッド)**
- **責務過多**: 
  - グローバル宣言登録（register_global_declarations: ~334行）
  - 構造体管理（struct定義、初期化、アクセス）
  - Interface実装管理（impl登録、検証）
  - 配列処理（多次元配列リテラル処理）
  - 関数呼び出し・戻り値処理
  - 型変換・キャスト
  - Union/Enum処理
  - デバッグ機能
  
- **分割候補**:
  1. 構造体管理 → `struct_manager.cpp`
  2. Interface管理 → `interface_manager.cpp`
  3. 関数呼び出し管理 → `function_manager.cpp`
  4. 型変換・キャスト → `type_conversion_manager.cpp`
  5. グローバル宣言登録 → `declaration_registry.cpp`

#### ❌ **expression_evaluator.cpp (5,869行、18メソッド)**
- **巨大メソッド**: `evaluate_expression_internal` が大部分を占める可能性
- **責務過多**:
  - 算術演算
  - 論理演算
  - 比較演算
  - 配列アクセス・スライス
  - 構造体アクセス
  - 関数呼び出し
  - 型変換
  - ポインタ操作
  
- **分割候補**:
  1. 算術・論理演算 → `arithmetic_evaluator.cpp`
  2. 配列式評価 → `array_expression_evaluator.cpp`
  3. 構造体式評価 → `struct_expression_evaluator.cpp`
  4. 関数呼び出し式評価 → `call_expression_evaluator.cpp`
  5. ポインタ式評価 → `pointer_expression_evaluator.cpp`

#### ❌ **variable_manager.cpp (4,057行、20メソッド)**
- **責務過多**:
  - 変数宣言・初期化
  - 変数アサイン（基本型、配列、構造体、ポインタ）
  - スコープ管理
  - グローバル変数管理
  - 静的変数管理
  - const変数管理
  
- **分割候補**:
  1. 基本型変数管理 → `primitive_variable_manager.cpp`
  2. 配列変数管理 → 既存の`array_manager.cpp`に統合
  3. 構造体変数管理 → `struct_variable_manager.cpp`
  4. ポインタ変数管理 → `pointer_variable_manager.cpp`
  5. スコープ管理 → `scope_manager.cpp`（独立）

#### ❌ **statement_executor.cpp (2,722行、15メソッド)**
- **責務過多**:
  - 変数宣言文
  - 代入文
  - if/while/for文
  - return文
  - 関数呼び出し文
  - print文
  
- **分割候補**:
  1. 変数宣言文実行 → `declaration_executor.cpp`
  2. 代入文実行 → `assignment_executor.cpp`
  3. 制御フロー実行 → `control_flow_executor.cpp`
  4. 関数呼び出し実行 → `call_executor.cpp`

#### ❌ **array_manager.cpp (1,912行、21メソッド)**
- **分割候補**:
  1. 配列アクセス → `array_access_manager.cpp`
  2. 配列初期化 → `array_initialization_manager.cpp`
  3. 多次元配列処理 → `multidim_array_manager.cpp`

#### ❌ **output_manager.cpp (1,371行)**
- **分割候補**:
  1. printf系 → `printf_manager.cpp`
  2. println系 → `println_manager.cpp`
  3. フォーマット処理 → `format_processor.cpp`

---

## 2. リファクタリング戦略

### 2.1 原則

1. **単一責務の原則（SRP）**: 1ファイル = 1つの明確な責務
2. **1ファイル1000行以内**: 技術的に困難な場合を除く
3. **後方互換性維持**: 既存のテストが全て通ること
4. **段階的リファクタリング**: 1ファイルずつ、テストを通しながら進める

### 2.2 優先順位

#### **Phase 1: interpreter.cpp の分割（最優先）**
目標: 5,896行 → 6ファイル × 約1,000行

1. `struct_manager.cpp` - 構造体定義・初期化・アクセス管理
2. `interface_manager.cpp` - Interface実装の登録・検証
3. `function_manager.cpp` - 関数呼び出し・戻り値処理
4. `type_conversion_manager.cpp` - 型変換・キャスト処理
5. `declaration_registry.cpp` - グローバル宣言の登録処理
6. `interpreter_core.cpp` - 残りのコアロジック（process, evaluate等）

**分割方針**:
- `interpreter.h`のpublicインターフェースは維持
- 内部実装を各Managerに委譲
- 各ManagerはInterpreterへのポインタを保持

#### **Phase 2: expression_evaluator.cpp の分割（最優先）**
目標: 5,869行 → 5ファイル × 約1,200行

1. `arithmetic_evaluator.cpp` - 算術・論理・比較演算
2. `array_expression_evaluator.cpp` - 配列アクセス・スライス式
3. `struct_expression_evaluator.cpp` - 構造体アクセス式
4. `call_expression_evaluator.cpp` - 関数呼び出し式
5. `pointer_expression_evaluator.cpp` - ポインタ式（参照外し、アドレス取得）

**分割方針**:
- `evaluate_expression_internal`の巨大なswitch文を分割
- 各Evaluatorはヘルパークラスとして実装
- ExpressionEvaluatorが各Evaluatorに委譲

#### **Phase 3: variable_manager.cpp の分割（高優先度）**
目標: 4,057行 → 5ファイル × 約800行

1. `scope_manager.cpp` - スコープ管理（push/pop/find）
2. `primitive_variable_manager.cpp` - 基本型変数の宣言・アサイン
3. `struct_variable_manager.cpp` - 構造体変数の宣言・アサイン
4. `pointer_variable_manager.cpp` - ポインタ変数の宣言・アサイン
5. `variable_manager_core.cpp` - 残りのコアロジック

**分割方針**:
- 既存の`array_manager.cpp`に配列変数管理を統合
- スコープ管理を独立させる（他でも使う）

#### **Phase 4: statement_executor.cpp の分割（高優先度）**
目標: 2,722行 → 4ファイル × 約680行

1. `declaration_executor.cpp` - 変数宣言文
2. `assignment_executor.cpp` - 代入文
3. `control_flow_executor.cpp` - if/while/for/return
4. `call_executor.cpp` - 関数呼び出し文

#### **Phase 5: array_manager.cpp の分割（中優先度）**
目標: 1,912行 → 3ファイル × 約640行

1. `array_access_manager.cpp` - 配列要素アクセス
2. `array_initialization_manager.cpp` - 配列初期化・リテラル
3. `multidim_array_manager.cpp` - 多次元配列処理

#### **Phase 6: output_manager.cpp の分割（中優先度）**
目標: 1,371行 → 3ファイル × 約450行

1. `printf_manager.cpp` - printf系
2. `println_manager.cpp` - println系
3. `format_processor.cpp` - フォーマット文字列処理

---

## 3. 新しいディレクトリ構造（リファクタリング後）

```
src/backend/interpreter/
├── core/                    # コアロジック
│   ├── interpreter_core.cpp/h    (~1,000行) ← 縮小
│   ├── type_inference.cpp/h      (722行) ← 不変
│   ├── error_handler.cpp/h       (315行) ← 不変
│   └── pointer_metadata.cpp/h    (334行) ← 不変
│
├── evaluator/               # 式評価 ← 拡張
│   ├── expression_evaluator.cpp/h        (~500行) ← 縮小（エントリポイント）
│   ├── arithmetic_evaluator.cpp/h        (~1,200行) ← 新規
│   ├── array_expression_evaluator.cpp/h  (~1,200行) ← 新規
│   ├── struct_expression_evaluator.cpp/h (~1,200行) ← 新規
│   ├── call_expression_evaluator.cpp/h   (~900行) ← 新規
│   └── pointer_expression_evaluator.cpp/h (~800行) ← 新規
│
├── executor/                # 文実行 ← 拡張
│   ├── statement_executor.cpp/h    (~500行) ← 縮小（エントリポイント）
│   ├── declaration_executor.cpp/h  (~680行) ← 新規
│   ├── assignment_executor.cpp/h   (~680行) ← 新規
│   ├── control_flow_executor.cpp/h (~680行) ← 新規
│   └── call_executor.cpp/h         (~180行) ← 新規
│
├── managers/                # データ管理 ← 拡張
│   ├── variable_manager_core.cpp/h       (~800行) ← 縮小
│   ├── scope_manager.cpp/h               (~400行) ← 新規
│   ├── primitive_variable_manager.cpp/h  (~800行) ← 新規
│   ├── struct_variable_manager.cpp/h     (~800行) ← 新規
│   ├── pointer_variable_manager.cpp/h    (~800行) ← 新規
│   │
│   ├── array_manager_core.cpp/h          (~500行) ← 縮小
│   ├── array_access_manager.cpp/h        (~640行) ← 新規
│   ├── array_initialization_manager.cpp/h (~640行) ← 新規
│   ├── multidim_array_manager.cpp/h      (~640行) ← 新規
│   │
│   ├── struct_manager.cpp/h              (~1,000行) ← 新規
│   ├── interface_manager.cpp/h           (~1,000行) ← 新規
│   ├── function_manager.cpp/h            (~1,000行) ← 新規
│   ├── type_conversion_manager.cpp/h     (~1,000行) ← 新規
│   ├── declaration_registry.cpp/h        (~1,000行) ← 新規
│   │
│   ├── type_manager.cpp/h                (516行) ← 不変
│   ├── enum_manager.cpp/h                (104行) ← 不変
│   └── common_operations.cpp/h           (518行) ← 不変
│
├── output/                  # 出力管理 ← 拡張
│   ├── output_manager.cpp/h    (~300行) ← 縮小（エントリポイント）
│   ├── printf_manager.cpp/h    (~450行) ← 新規
│   ├── println_manager.cpp/h   (~450行) ← 新規
│   └── format_processor.cpp/h  (~450行) ← 新規
│
└── services/                # ヘルパーサービス
    ├── expression_service.cpp/h        (150行) ← 不変
    ├── variable_access_service.cpp/h   (185行) ← 不変
    ├── debug_service.cpp/h             (165行) ← 不変
    └── array_processing_service.cpp/h  (388行) ← 不変
```

**変更後の総行数**: 約27,000行（ヘッダー追加により若干増加）  
**ファイル数**: 22ファイル → 46ファイル（約2倍）  
**1ファイル平均**: 約600行（目標1000行以内達成）

---

## 4. 各Phaseの詳細計画

### Phase 1: interpreter.cpp の分割

#### 4.1.1 struct_manager.cpp
**責務**: 構造体の定義、初期化、メンバーアクセス管理

**移行対象メソッド**:
- `register_struct_definition()`
- `find_struct_definition()`
- `create_struct_instance()`
- `assign_struct_member()`
- `get_struct_member_value()`
- 構造体リテラル処理
- 構造体コピー・代入

**推定行数**: 約1,000行

#### 4.1.2 interface_manager.cpp
**責務**: Interface実装の登録、検証、メソッド呼び出し

**移行対象メソッド**:
- `handle_impl_declaration()`
- `find_impl_method()`
- `validate_interface_implementation()`
- Interface viewの管理

**推定行数**: 約1,000行

#### 4.1.3 function_manager.cpp
**責務**: 関数呼び出し、戻り値処理、パラメータバインディング

**移行対象メソッド**:
- 関数呼び出し処理（複数オーバーロード）
- `assign_function_parameter()`
- 戻り値の型変換・アサイン
- 関数ポインタ処理

**推定行数**: 約1,000行

#### 4.1.4 type_conversion_manager.cpp
**責務**: 型変換、キャスト、型チェック

**移行対象メソッド**:
- 明示的キャスト処理
- 暗黙的型変換
- `check_type_range()`
- ポインタ型変換

**推定行数**: 約1,000行

#### 4.1.5 declaration_registry.cpp
**責務**: グローバル宣言の登録（関数、構造体、enum、interface、union）

**移行対象メソッド**:
- `register_global_declarations()` (~334行)
- 各種定義の登録処理

**推定行数**: 約900行

#### 4.1.6 interpreter_core.cpp
**責務**: 残りのコアロジック（process, evaluate, execute_statement等のエントリポイント）

**残るメソッド**:
- `process()`
- `evaluate()`
- `evaluate_typed()`
- `execute_statement()` ← 委譲のみ
- 初期化処理

**推定行数**: 約1,000行

---

### Phase 2: expression_evaluator.cpp の分割

#### 4.2.1 arithmetic_evaluator.cpp
**責務**: 算術演算、論理演算、比較演算

**移行対象**:
- `+`, `-`, `*`, `/`, `%` 演算
- `&&`, `||`, `!` 論理演算
- `<`, `>`, `<=`, `>=`, `==`, `!=` 比較演算
- ビット演算 (`&`, `|`, `^`, `<<`, `>>`)

**推定行数**: 約1,200行

#### 4.2.2 array_expression_evaluator.cpp
**責務**: 配列アクセス、配列スライス式

**移行対象**:
- 配列要素アクセス（1次元、多次元）
- 配列スライス
- 配列リテラル式

**推定行数**: 約1,200行

#### 4.2.3 struct_expression_evaluator.cpp
**責務**: 構造体メンバーアクセス式

**移行対象**:
- ドットアクセス (`struct.member`)
- アロー演算子 (`ptr->member`)
- ネストした構造体アクセス

**推定行数**: 約1,200行

#### 4.2.4 call_expression_evaluator.cpp
**責務**: 関数呼び出し式

**移行対象**:
- 関数呼び出し式の評価
- 引数評価・バインディング
- 戻り値の取得

**推定行数**: 約900行

#### 4.2.5 pointer_expression_evaluator.cpp
**責務**: ポインタ式（参照外し、アドレス取得）

**移行対象**:
- `*ptr` (参照外し)
- `&var` (アドレス取得)
- ポインタ演算 (`ptr + offset`)

**推定行数**: 約800行

---

### Phase 3: variable_manager.cpp の分割

#### 4.3.1 scope_manager.cpp
**責務**: スコープのpush/pop、変数検索

**移行対象**:
- `push_scope()`
- `pop_scope()`
- `find_variable()` (スコープチェーン探索)
- スコープスタック管理

**推定行数**: 約400行

#### 4.3.2 primitive_variable_manager.cpp
**責務**: 基本型（int, float, bool等）の変数宣言・アサイン

**移行対象**:
- 基本型変数の宣言
- 基本型変数へのアサイン
- const変数チェック

**推定行数**: 約800行

#### 4.3.3 struct_variable_manager.cpp
**責務**: 構造体変数の宣言・アサイン

**移行対象**:
- 構造体変数の宣言
- 構造体変数へのアサイン
- 構造体コピー

**推定行数**: 約800行

#### 4.3.4 pointer_variable_manager.cpp
**責務**: ポインタ変数の宣言・アサイン

**移行対象**:
- ポインタ変数の宣言
- ポインタ変数へのアサイン
- nullptr処理

**推定行数**: 約800行

#### 4.3.5 variable_manager_core.cpp
**責務**: 残りのコアロジック、統合管理

**残るメソッド**:
- エントリポイント（各Managerへの委譲）
- グローバル変数管理
- 静的変数管理

**推定行数**: 約800行

---

### Phase 4以降は同様の方針で段階的に進める

---

## 5. 実装手順（Phase 1の例）

### 5.1 準備
1. ✅ 現状のテストが全て通ることを確認
2. ✅ `docs/interpreter_refactoring_plan.md` を作成（このドキュメント）
3. ブランチ作成: `git checkout -b refactor/interpreter-phase1`

### 5.2 struct_manager.cpp の作成
1. `src/backend/interpreter/managers/struct_manager.h` 作成
   - クラス定義
   - Interpreterへのポインタを保持
2. `src/backend/interpreter/managers/struct_manager.cpp` 作成
   - interpreter.cppから構造体関連メソッドをコピー
   - 名前空間を調整
3. interpreter.hに`struct_manager_`メンバー追加
4. interpreter.cppのコンストラクタで初期化
5. interpreter.cppの構造体関連メソッドを委譲に書き換え
6. ビルド・テスト確認
7. コミット: `git commit -m "refactor: Extract struct_manager from interpreter"`

### 5.3 interface_manager.cpp の作成
（同様の手順）

### 5.4 以降のManagerも同様に進める

### 5.5 Phase 1完了後
1. 全テストが通ることを確認
2. コードレビュー
3. マージ: `git merge refactor/interpreter-phase1`
4. Phase 2へ進む

---

## 6. テスト戦略

### 6.1 テストの種類
1. **統合テスト**: `make test` で実行される既存のテスト（2,380個）
2. **単体テスト**: 各Managerの単体テスト（必要に応じて追加）

### 6.2 各Phase後のチェックリスト
- [ ] 全統合テストが通る（2,380/2,380）
- [ ] ビルド警告がない
- [ ] 新しいManagerクラスが正しく初期化されている
- [ ] メモリリークがない（valgrindで確認）
- [ ] パフォーマンス劣化がない（必要に応じて計測）

---

## 7. リスク管理

### 7.1 予想されるリスク
1. **循環依存**: Manager間で循環依存が発生する可能性
   - 対策: 前方宣言、インターフェース分離
2. **テスト失敗**: リファクタリングでテストが失敗する
   - 対策: 小さいステップで進め、各ステップでテスト
3. **パフォーマンス劣化**: 委譲による関数呼び出しオーバーヘッド
   - 対策: inline関数、必要に応じて最適化

### 7.2 ロールバック戦略
- 各Phaseごとにブランチを切る
- Phase完了後にマージ
- 問題があればブランチを削除してやり直し

---

## 8. スケジュール

| Phase | 作業内容 | 推定工数 | 優先度 |
|-------|---------|---------|--------|
| **Phase 1** | interpreter.cpp分割 | 3-4日 | 最優先 |
| **Phase 2** | expression_evaluator.cpp分割 | 3-4日 | 最優先 |
| **Phase 3** | variable_manager.cpp分割 | 2-3日 | 高 |
| **Phase 4** | statement_executor.cpp分割 | 2日 | 高 |
| **Phase 5** | array_manager.cpp分割 | 1-2日 | 中 |
| **Phase 6** | output_manager.cpp分割 | 1日 | 中 |

**合計推定工数**: 12-16日

---

## 9. 成功基準

1. ✅ 全ファイルが1000行以内（技術的に困難な場合を除く）
2. ✅ 全統合テストが通る（2,380/2,380）
3. ✅ ビルド警告がない
4. ✅ 各ファイルの責務が明確
5. ✅ 新規開発者が理解しやすいコード構造

---

## 10. 実装ログ

### Phase 1: interpreter.cpp の分割（進行中）

#### 作業開始: 2025年10月7日

**方針変更**: 
パーサーリファクタリングで学んだ教訓を活かし、より段階的なアプローチを採用。
巨大なファイルを一度に分割するのではなく、以下の順序で進める：

1. **最も独立性の高い機能から分離** - 依存関係が少ない部分
2. **既存のManagerクラスとの整合性** - variable_manager等との統合
3. **段階的なテストと検証** - 各ステップでビルド・テスト確認

#### 修正された優先順位:

**Phase 1A: 出力管理の整理（最優先）**
- output_manager.cpp (1,371行) は既に分離されているが、さらに細分化
- printf_manager, println_manager, format_processor に分割
- 理由: 他の部分への依存が少なく、分離が容易

**Phase 1B: 型システムの整理**
- type_inference.cpp (722行) は適切なサイズだが、type_conversion機能を分離
- type_conversion_manager.cpp を作成
- interpreter.cppの型変換ロジックを移行

**Phase 1C: 構造体管理の分離**
- struct_manager.cpp の作成（開始済み）
- interpreter.cppから構造体関連メソッドを段階的に移行

**Phase 1D: Interface管理の分離**  
- interface_manager.cpp の作成
- impl定義管理を分離

---

**作成者**: GitHub Copilot  
**ステータス**: Phase 1A 実装中
