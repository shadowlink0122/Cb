# Interpreter アーキテクチャ

**作成日**: 2025年10月9日  
**対象バージョン**: v0.9.2-dev  
**最終更新**: Phase 1 完了時点

---

## 概要

Cb言語のインタープリターは、モジュラー設計を採用し、各機能を専門化したマネージャーやサービスに分離しています。Phase 1 (v0.9.2) では、`interpreter.cpp`を機能別に分割し、保守性を向上させました。

---

## ファイル構成

### Core Files (`src/backend/interpreter/core/`)

#### interpreter.cpp (1,722行)
**役割**: コア実行ロジックと調整役

**主要機能**:
- `process()` - メイン処理ループ
- `execute_statement()` - 文の実行
- `evaluate()`, `evaluate_typed()` - 式の評価
- `register_global_declarations()` - グローバル宣言の登録

**特徴**:
- 各機能はマネージャーへ委譲
- 薄いラッパー関数が多数（1-3行）
- 調整役としての責務に集中

#### initialization.cpp (144行) ✨ **v0.9.2で新規作成**
**役割**: インタープリターの初期化処理

**主要機能**:
- `Interpreter::Interpreter()` - コンストラクタ
  - 21個のマネージャー/サービスを初期化
  - unique_ptrでメモリ安全性を確保
- `initialize_global_variables()` - グローバル変数初期化
- `sync_enum_definitions_from_parser()` - enum定義同期

**依存関係**:
```cpp
// マネージャー初期化順序
output_manager_          // 出力管理
variable_manager_        // 変数管理
type_manager_            // 型管理
expression_evaluator_    // 式評価
array_manager_           // 配列管理
statement_executor_      // 文実行
// ... 他16個のマネージャー/サービス
```

#### cleanup.cpp (36行) ✨ **v0.9.2で新規作成**
**役割**: スコープと一時変数の管理

**主要機能**:
- `push_scope()` - 新しいスコープ開始
- `pop_scope()` - スコープ終了
- `current_scope()` - 現在のスコープレベル取得
- `add_temp_variable()` - 一時変数追加
- `remove_temp_variable()` - 一時変数削除
- `clear_temp_variables()` - 一時変数全クリア

**注意**:
- デストラクタは`interpreter.cpp`に残存
- 理由: unique_ptrの完全な型定義が必要

#### utility.cpp (156行) ✨ **v0.9.2で新規作成**
**役割**: 横断的ユーティリティ関数

**主要機能**:

1. **型解決** (4関数)
   - `resolve_typedef()` - typedef解決
   - `resolve_type_alias()` - 型エイリアス解決
   - `string_to_type_info()` - 文字列→TypeInfo変換
   - `check_type_range()` - 型範囲チェック

2. **配列ヘルパー** (3関数)
   - `extract_array_name()` - 配列名抽出
   - `extract_array_indices()` - 配列インデックス抽出
   - `extract_array_element_name()` - 配列要素名抽出

3. **変数検索** (3関数)
   - `find_variable()` - 変数検索
   - `find_variable_name()` - 変数名検索（ポインタから）
   - `find_variable_name_by_address()` - アドレスから変数名検索

4. **Static変数管理** (7関数)
   - `add_static_variable()` - static変数追加
   - `find_static_variable()` - static変数検索
   - `clear_static_variables()` - static変数クリア
   - `add_impl_static_variable()` - impl static変数追加
   - `find_impl_static_variable()` - impl static変数検索
   - `get_all_impl_static_variables()` - 全impl static変数取得
   - `clear_impl_static_variables()` - impl static変数クリア

5. **エラー報告** (2関数)
   - `throw_runtime_error_with_location()` - 位置情報付きエラー
   - `print_error_at_node()` - ノード位置でエラー表示

---

## 分割の効果

### Before (v0.9.1以前)
```
interpreter.cpp: 1,941行
- 初期化、実行、ユーティリティが混在
- 巨大なファイルで保守困難
```

### After (v0.9.2)
```
interpreter.cpp:      1,722行 (-194行, -10%)
initialization.cpp:     144行 (新規)
cleanup.cpp:             36行 (新規)
utility.cpp:            156行 (新規)
------------------------------------------
合計:                 2,058行 (+117行)
```

**メリット**:
- ✅ 責任分離の明確化
- ✅ 関数の発見性向上
- ✅ テストのしやすさ向上
- ✅ コンパイル時間の短縮（部分コンパイル）

---

## マネージャー構成

interpreter.cppは以下のマネージャーに機能を委譲しています：

### 1. 変数管理 (`managers/variables/`)
- **VariableManager** - 変数の作成、代入、検索
- 関連ファイル:
  - `manager.cpp` - メイン機能
  - `assignment.cpp` - 代入処理
  - `declaration.cpp` - 宣言処理
  - `initialization.cpp` - 初期化処理
  - `static.cpp` - static変数管理

### 2. 配列管理 (`managers/arrays/`)
- **ArrayManager** - 配列の作成、アクセス、コピー
- 関連ファイル:
  - `manager.cpp` (2,107行) - 配列の全機能

### 3. 構造体管理 (`managers/structs/`)
- **StructOperations** - 構造体の基本操作
- **StructVariableManager** - 構造体変数管理
- **StructAssignmentManager** - 構造体代入
- **StructSyncManager** - 構造体同期
- 関連ファイル:
  - `operations.cpp` - 基本操作
  - `member_variables.cpp` - メンバー変数管理
  - `assignment.cpp` - 代入処理
  - `sync.cpp` - 同期処理

### 4. 型管理 (`managers/types/`)
- **TypeManager** - 型定義、解決
- **EnumManager** - enum管理
- **InterfaceOperations** - interface/impl管理
- 関連ファイル:
  - `manager.cpp` - 型管理
  - `enums.cpp` - enum処理
  - `interfaces.cpp` - interface処理

### 5. 出力管理 (`output/`)
- **OutputManager** - 標準出力、フォーマット出力
- 関連ファイル:
  - `output_manager.cpp` - 出力機能全般

### 6. サービス層 (`services/`)
- **ExpressionService** - 式評価サポート
- **VariableAccessService** - 変数アクセス統一
- **ArrayProcessingService** - 配列処理統一
- **DebugService** - デバッグ機能統一

---

## 委譲パターン

interpreter.cppの多くの関数は、適切なマネージャーへの薄いラッパーです：

### 例1: 変数代入
```cpp
// interpreter.cpp (ラッパー)
void Interpreter::assign_variable(const std::string &name, int64_t value,
                                  TypeInfo type) {
    variable_manager_->assign_variable(name, value, type, false);
}

// 実装は managers/variables/assignment.cpp
```

### 例2: 構造体操作
```cpp
// interpreter.cpp (ラッパー)
void Interpreter::register_struct_definition(
    const std::string &struct_name, const StructDefinition &definition) {
    struct_operations_->register_struct_definition(struct_name, definition);
}

// 実装は managers/structs/operations.cpp
```

### 例3: 配列処理
```cpp
// interpreter.cpp (一部ロジックあり)
void Interpreter::assign_array_element(const std::string &name, int64_t index,
                                       int64_t value) {
    // エラーハンドリングと境界チェック
    Variable *var = find_variable(name);
    if (!var) {
        throw std::runtime_error("Undefined array");
    }
    
    // 実装は common_operations_ に委譲
    common_operations_->assign_array_element_safe(var, index, value, name);
}
```

---

## 型ヘルパーシステム (v0.9.2)

### TypeHelpers (`src/common/type_helpers.h`)

Phase 1で拡張された型ユーティリティ関数群：

```cpp
// 既存の型判定関数
bool isInteger(TypeInfo type);
bool isFloating(TypeInfo type);
bool isNumeric(TypeInfo type);
bool isStruct(TypeInfo type);
// ... 他多数

// v0.9.2で追加された関数
bool needsExplicitCast(TypeInfo from, TypeInfo to);
TypeInfo getCommonNumericType(TypeInfo type1, TypeInfo type2);
size_t getTypeSize(TypeInfo type);
size_t getTypeAlignment(TypeInfo type);
bool isSignedInteger(TypeInfo type);
int64_t getTypeMinValue(TypeInfo type, bool is_unsigned = false);
int64_t getTypeMaxValue(TypeInfo type, bool is_unsigned = false);
```

**利点**:
- コード重複削減（242箇所中140-150箇所を置換、58-62%削減）
- 可読性向上（冗長な型チェックが簡潔に）
- 保守性向上（型チェックロジックが一箇所に集約）
- ゼロオーバーヘッド（inline関数）

---

## エラーレポートシステム (v0.9.2)

### ErrorReporter (`src/common/error_reporter.h`)

Phase 1で新規実装された詳細なエラー報告システム：

```cpp
// 4段階の重要度レベル
enum class ErrorSeverity {
    NOTE,    // 情報
    WARNING, // 警告
    ERROR,   // エラー
    FATAL    // 致命的
};

// エラー報告
void report(ErrorSeverity severity, SourceLocation loc, 
            const std::string &message,
            const std::vector<std::string> &suggestions = {});
```

**出力例**:
```
test.cb:3:13: error: Undefined variable 'unknown_var'
   2 |     int x = 10;
   3 |     int y = unknown_var;
     |             ^
   4 |     return 0;

Did you mean one of these?
  - known_var
  - x
  - y
```

**機能**:
- ソースコード引用（前後の行も表示）
- カレット記号（^）でエラー位置を明示
- "Did you mean?" 提案（Levenshtein距離ベース）
- 色付き出力対応

---

## コーディング規約

### 1. メモリ管理
- ✅ **使用**: `std::unique_ptr`, `std::shared_ptr`
- ❌ **禁止**: 生の`new`/`delete`（特別な理由がない限り）

### 2. 関数サイズ
- 推奨: 50行以下
- 最大: 100行（超える場合は分割を検討）

### 3. ファイルサイズ
- 推奨: 500行以下
- 許容: 2,000行以下
- 超過: 3,000行で分割を計画

### 4. 型チェック
- ✅ **使用**: `TypeHelpers::isInteger(type)`
- ❌ **非推奨**: `type == TYPE_INT || type == TYPE_LONG || ...`

### 5. エラー報告
- 将来的に`ErrorReporter`を使用
- 現在は従来の`throw std::runtime_error()`も許容

---

## テスト構成

### 統合テスト (`tests/integration/`)
- 30個のテストケース
- 実行時間: 平均3.5秒
- カバレッジ: 主要機能全般

### ユニットテスト (`tests/unit/`)
- 50個のテストケース
- ErrorReporter: 9テスト
- 他: 41テスト

### テスト追加 (v0.9.2 Phase 1)
- エラーハンドリング: 5テスト
- リグレッション: 2テスト
- エッジケース: 3テスト
- 関数ポインタ・型変換: 3テスト

---

## パフォーマンス指標

### ベンチマーク結果 (v0.9.2)
| プログラム | 実行時間 |
|-----------|---------|
| fibonacci.cb | 0.03秒 |
| dijkstra_struct.cb | 0.03秒 |
| knapsack_dp.cb | 0.02秒 |

### ビルド時間
- クリーンビルド: 約5-8秒（並列コンパイル -j8）
- 部分ビルド: 1-2秒（変更ファイルのみ）

---

## 今後の改善計画

### Phase 2以降
1. **大規模ファイルの分割**
   - `call_impl.cpp` (2,129行) → 4ファイルに分割
   - `arrays/manager.cpp` (2,107行) → 4ファイルに分割

2. **ErrorReporter統合**
   - Token構造体に列番号追加
   - インタープリター全体で使用

3. **パフォーマンス最適化**
   - プロファイリングベースの最適化
   - ホットパスの特定と改善

4. **テストカバレッジ拡大**
   - カバレッジ計測ツール導入
   - 80%以上のカバレッジ目標

---

## 参考資料

- [v0.9.2 実装チェックリスト](../todo/v0.9.2_checklist.md)
- [v0.9.2 進捗レポート](../todo/v0.9.2_progress_report.md)
- [Phase 8 実装計画](../todo/phase8_implementation_plan.md)
- [BNF仕様](../BNF.md)
- [言語仕様](../spec.md)

---

**作成者**: GitHub Copilot  
**レビュー日**: 2025年10月9日
