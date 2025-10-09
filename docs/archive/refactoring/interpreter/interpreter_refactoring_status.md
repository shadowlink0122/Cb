# Interpreter Refactoring - Status Report

**日付**: 2025年10月7日  
**ブランチ**: feature/pointer2  
**ステータス**: Phase 1 計画完了、実装準備中

---

## 完了した作業

### 1. ExpressionEvaluator分割（完了）
- **削減**: 3,294行 → 985行（70%削減）
- **方法**: ExpressionDispatcherパターン
- **ファイル構成**:
  - `expression_dispatcher.cpp` (271行) - 中央ディスパッチャー
  - `expression_function_call_impl.cpp` (2,089行) - 関数呼び出し
  - `expression_member_access_impl.cpp` (583行) - メンバーアクセス
  - その他のヘルパーファイル
- **テスト結果**: ✅ 2,380/2,380統合テスト + 30/30ユニットテスト通過

### 2. 関数ポインタコールバック修正（完了）
- **問題**: `operation(x, y)`形式の関数ポインタ呼び出しが失敗
- **解決**: `expression_dispatcher.cpp`の`AST_FUNC_CALL`処理を簡略化
- **テスト結果**: ✅ 2,333/2,333テスト通過

---

## 現状分析: interpreter.cpp

### 基本情報
- **総行数**: 6,752行
- **目標**: ~1,500行（78%削減）
- **現在のビルド**: 正常
- **全テスト**: 通過

### 関数サイズ分析

| 関数名 | 行数（概算） | 優先度 | 抽出先候補 |
|--------|-------------|--------|-----------|
| `register_global_declarations` | 352行 | 中 | initialization_manager |
| Struct関連（複数） | ~2,500行 | 高 | struct_operations |
| Interface関連（複数） | ~500行 | 中 | interface_operations |
| 配列・変数代入関連 | ~600行 | 低 | 既存Managerへ |
| その他 | ~2,800行 | - | interpreter_core |

### Struct関連の関数一覧（抽出候補）

**定義・検証**:
- `register_struct_definition()` (3011行～)
- `validate_struct_recursion_rules()` (3023行～)

**変数作成**:
- `create_struct_variable()` (3406行～)
- `create_struct_member_variables_recursively()` (3930行～)

**代入・アクセス**:
- `assign_struct_literal()` (4032行～)
- `assign_struct_member()` (4804行～) - 3つのオーバーロード
- `assign_struct_member_struct()` (5186行～)
- `assign_struct_member_array_element()` (5276行～) - 2つのオーバーロード
- `assign_struct_member_array_literal()` (5586行～)

**配列メンバーアクセス**:
- `get_struct_member_array_element()` (行番号不明)
- `get_struct_member_multidim_array_element()` (行番号不明)
- `get_struct_member_array_string_element()` (行番号不明)

**同期処理**:
- `sync_struct_definitions_from_parser()` (5841行～)
- `sync_struct_members_from_direct_access()` (5858行～)
- `sync_direct_access_from_struct_value()` (6289行～)
- `sync_individual_member_from_struct()` (3245行～)

**アクセス制御**:
- `ensure_struct_member_access_allowed()` (3322行～)
- `is_current_impl_context_for()` (2810行付近)

**推定合計**: ~2,500行

---

## 実装戦略の修正

### 元の計画
大きな機能ブロック（Struct、Interface等）を新しいクラスに抽出

### 修正後の計画（より実用的）
段階的に進める：

#### ✅ Step 1: ドキュメント作成（完了）
- リファクタリング計画の文書化
- 現状分析の完了
- 優先順位の決定

#### 📋 Step 2: コード整理（次のステップ）
interpreter.cpp内で関数を機能ごとにグループ化（コメント追加）

**方法**:
```cpp
// ========================================================================
// Struct Definition & Validation
// ========================================================================

void Interpreter::register_struct_definition(...) { ... }
void Interpreter::validate_struct_recursion_rules() { ... }

// ========================================================================
// Struct Variable Creation
// ========================================================================

void Interpreter::create_struct_variable(...) { ... }
...
```

**メリット**:
- コードの可読性向上
- 将来の分割が容易
- リスクなし（テストそのまま通る）

#### 📋 Step 3: 小さい機能の抽出
最も独立した小さい機能から抽出：

1. **Static変数管理** (~200行)
   - `find_static_variable()`
   - `create_static_variable()`
   - `find_impl_static_variable()`
   - `create_impl_static_variable()`
   - → `static_variable_manager.cpp`

2. **型解決** (~100行)
   - `resolve_typedef()` → TypeManagerに移動
   - `resolve_type_alias()` → TypeManagerに移動
   - `string_to_type_info()` → TypeManagerに移動

3. **エラー処理** (~50行)
   - `check_type_range()` → ErrorHandlerに移動
   - `throw_runtime_error_with_location()` → ErrorHandlerに移動
   - `print_error_at_node()` → ErrorHandlerに移動

**推定削減**: ~350行（6,752行 → 6,402行）

#### 📋 Step 4: 中規模機能の抽出
**Interface Operations** (~500行)
- interface関連の関数を`interface_operations.cpp`に移動

**推定削減**: ~500行（6,402行 → 5,902行）

#### 📋 Step 5: 大規模機能の抽出（最後）
**Struct Operations** (~2,500行)
- 全struct関連の関数を`struct_operations.cpp`に移動
- これは最も大きい作業なので、段階的に進める

**推定削減**: ~2,500行（5,902行 → 3,402行）

#### 📋 Step 6: 最終調整
- 不要なコード削除
- コメント整理
- パフォーマンス最適化

**最終目標**: ~1,500行

---

## 次のアクション

### 即座に実施可能（低リスク）

1. **コード整理**
   - interpreter.cpp内に機能ブロックのコメントを追加
   - 関数を機能ごとにグループ化
   - 所要時間: 30分
   - リスク: なし

2. **型解決関数の移動**
   - `resolve_typedef()`等をTypeManagerに移動
   - 所要時間: 1時間
   - リスク: 低（既存のManagerへの追加）

3. **Static変数管理の抽出**
   - 新しいクラス`StaticVariableManager`を作成
   - 所要時間: 2時間
   - リスク: 低（独立した機能）

### 優先順位
1. 🟢 **高優先度・低リスク**: コード整理（Step 2）
2. 🟢 **高優先度・低リスク**: 小さい機能の抽出（Step 3）
3. 🟡 **中優先度・中リスク**: Interface Operations抽出（Step 4）
4. 🔴 **高優先度・高リスク**: Struct Operations抽出（Step 5）

---

## 学んだ教訓（ExpressionEvaluatorリファクタリングから）

1. **段階的なアプローチが重要**
   - 一度に大きな変更をしない
   - 各ステップでテストを実行

2. **明確なインターフェース**
   - Dispatcherパターンが有効
   - 委譲を使って既存のAPIを維持

3. **テストの重要性**
   - リファクタリング前にテストが全て通ることを確認
   - 各ステップ後にテストを実行

4. **ドキュメンテーション**
   - 変更内容を記録
   - 将来の参考のため

---

## 推奨される次のステップ

**提案**: Step 2（コード整理）から始める

**理由**:
- リスクなし（コメント追加のみ）
- 即座に可読性向上
- 将来の分割作業が容易に
- 所要時間が短い（30分）

**実装方法**:
interpreter.cppに機能ブロックを示すコメントを追加：

```cpp
// ========================================================================
// SECTION 1: Initialization & Global Declarations
// ========================================================================
// - register_global_declarations()
// - initialize_global_variables()
// - sync_*_from_parser()

// ========================================================================
// SECTION 2: Struct Operations (2,500 lines)
// ========================================================================
// - register_struct_definition()
// - validate_struct_recursion_rules()
// - create_struct_variable()
// - assign_struct_*()
// - get_struct_*()
// - sync_struct_*()

// ========================================================================
// SECTION 3: Interface Operations (500 lines)
// ========================================================================
// - register_interface_definition()
// - register_impl_definition()
// - handle_impl_declaration()
// - enter_impl_context()
// - exit_impl_context()

// ========================================================================
// SECTION 4: Variable Management
// ========================================================================
// - assign_variable() (multiple overloads)
// - assign_function_parameter()
// - assign_array_*()

// ========================================================================
// SECTION 5: Array Operations
// ========================================================================
// - getMultidimensionalArrayElement()
// - setMultidimensionalArrayElement()
// - extract_array_*()

// ========================================================================
// SECTION 6: Static Variables
// ========================================================================
// - find_static_variable()
// - create_static_variable()
// - find_impl_static_variable()
// - create_impl_static_variable()

// ========================================================================
// SECTION 7: Type Resolution
// ========================================================================
// - resolve_typedef()
// - resolve_type_alias()
// - string_to_type_info()

// ========================================================================
// SECTION 8: Error Handling
// ========================================================================
// - check_type_range()
// - throw_runtime_error_with_location()
// - print_error_at_node()

// ========================================================================
// SECTION 9: Core Functions
// ========================================================================
// - Constructor/Destructor
// - process()
// - evaluate()
// - execute_statement()
// - Scope management
```

**次のコマンド**: この構造を実装するか、他の提案があればお知らせください。

---

**作成者**: GitHub Copilot  
**最終更新**: 2025年10月7日
