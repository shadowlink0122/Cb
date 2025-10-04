# ポインタ機能強化実装計画

## PRタイトル案

### Phase 1 (現在進行中)
**"feat: Enhance pointer system with comprehensive address-of support"**
- 配列要素へのアドレス取得サポート
- 構造体メンバーへのアドレス取得サポート
- 包括的なテストケースの追加

### Phase 2 (次のPR)
**"feat: Implement pointer metadata system for array and struct element access"**
- ポインタメタデータ構造の導入
- 配列要素への間接参照の修正
- 構造体メンバーへの間接参照の修正

### Phase 3 (将来のPR)
**"feat: Add pointer arithmetic and array traversal support"**
- ポインタ演算（+, -, ++, --）の実装
- 配列走査のサポート

## 実装計画詳細

### Phase 1: 基盤整備とテスト拡充（当面の目標）

#### 1.1 配列初期化バグの修正
**ファイル**: `src/backend/interpreter/executor/statement_executor.cpp`
**問題**: 配列リテラル `{1, 2, 3}` が正しく設定されない
**タスク**:
- [ ] `execute_array_decl()` の配列リテラル処理を確認
- [ ] グローバル配列の初期化を確認
- [ ] ローカル配列の初期化を確認
- [ ] テストケース作成

#### 1.2 アドレス取得機能の完成
**ファイル**: `src/backend/interpreter/evaluator/expression_evaluator.cpp`
**現状**: 
- ✅ 変数のアドレス取得
- ✅ 配列要素のアドレス取得（アドレスは取得できるが間接参照が壊れる）
- ✅ 構造体メンバーのアドレス取得（アドレスは取得できるが間接参照が壊れる）

**タスク**:
- [x] `ADDRESS_OF` 演算子の拡張（完了）
- [ ] 型付き評価での対応確認
- [ ] エラーハンドリングの改善

#### 1.3 テストケースの拡充
**新規テストファイル**:
- [x] `test_struct_pointer_members.cb` - 構造体メンバーとしてのポインタ
- [x] `test_impl_with_pointers.cb` - implブロック内でのポインタ使用
- [x] `test_pointer_return_advanced.cb` - ポインタを返す関数の高度なテスト
- [x] `test_address_comprehensive.cb` - アドレス取得の包括テスト
- [x] `test_pointer_arithmetic.cb` - ポインタ演算（未実装機能の準備）
- [x] `test_reference_parameters.cb` - 参照引数の包括テスト

**既存テストの更新**:
- [ ] 配列初期化を使用しているすべてのテストを確認
- [ ] ポインタテストの統合テストスイートへの追加

#### 1.4 ドキュメント整備
- [x] `POINTER_KNOWN_ISSUES.md` - 既知の問題と理論上のバグ
- [ ] `POINTER_IMPLEMENTATION_STATUS.md` - 実装状況のサマリ
- [ ] `POINTER_API_REFERENCE.md` - ポインタAPIのリファレンス

### Phase 2: ポインタメタデータシステム（次のPR）

#### 2.1 メタデータ構造の設計
**新規ファイル**: `src/backend/interpreter/core/pointer_metadata.h`

```cpp
enum class PointerTarget {
    VARIABLE,       // 通常の変数
    ARRAY_ELEMENT,  // 配列要素
    STRUCT_MEMBER,  // 構造体メンバー
    NULLPTR         // nullポインタ
};

struct PointerMetadata {
    PointerTarget target;
    
    // 対象の情報
    union {
        struct {
            Variable* var;
        } variable;
        
        struct {
            Variable* array;
            size_t index;
            TypeInfo element_type;
        } array_element;
        
        struct {
            Variable* member;
            std::string member_name;
        } struct_member;
    } data;
    
    // ヘルパー関数
    static PointerMetadata from_variable(Variable* var);
    static PointerMetadata from_array_element(Variable* array, size_t index, TypeInfo type);
    static PointerMetadata from_struct_member(Variable* member, const std::string& name);
    static PointerMetadata null_pointer();
    
    int64_t read_value() const;
    void write_value(int64_t value);
    double read_float_value() const;
    void write_float_value(double value);
};
```

#### 2.2 後方互換性の維持
**戦略**:
1. 既存の変数ポインタは引き続き`Variable*`として動作
2. 新しいメタデータは配列/構造体要素にのみ使用
3. 移行期間中は両方の方式をサポート

**実装方法**:
```cpp
// Option 1: タグ付きユニオン（推奨）
struct PointerValue {
    bool is_simple_variable;
    union {
        Variable* simple_var;  // 従来の方式
        PointerMetadata* metadata;  // 新しい方式
    };
};

// Option 2: 常にメタデータを使用（完全移行）
// 変数ポインタもメタデータで包む
```

#### 2.3 間接参照の修正
**ファイル**: `src/backend/interpreter/evaluator/expression_evaluator.cpp`
**タスク**:
- [ ] `DEREFERENCE` 演算子の拡張
- [ ] メタデータに基づく読み取り
- [ ] メタデータに基づく書き込み
- [ ] 型安全性のチェック

#### 2.4 代入演算子の修正
**ファイル**: `src/backend/interpreter/executor/statement_executor.cpp`
**タスク**:
- [ ] `*ptr = value` の処理を拡張
- [ ] 型変換の適切な処理
- [ ] エラーハンドリング

### Phase 3: ポインタ演算（将来のPR）

#### 3.1 算術演算の実装
**演算子**:
- `ptr + n` / `ptr - n`: オフセット計算
- `ptr++` / `++ptr` / `ptr--` / `--ptr`: インクリメント/デクリメント
- `ptr1 - ptr2`: ポインタ間の距離

**タスク**:
- [ ] 型サイズを考慮したオフセット計算
- [ ] 配列境界チェック（オプション）
- [ ] ポインタ比較演算

#### 3.2 配列走査の最適化
**パターン**:
```cb
int[100] arr;
int* ptr = &arr[0];
for (int i = 0; i < 100; i++) {
    *ptr = i;
    ptr++;
}
```

## テスト戦略

### Phase 1のテスト
- [x] 基本型のアドレス取得と間接参照
- [x] 構造体メンバーポインタ
- [x] implブロック内でのポインタ使用
- [ ] 配列初期化の検証
- [ ] エッジケース（nullptr, 多重ポインタ）

### Phase 2のテスト
- [ ] 配列要素への間接参照
- [ ] 構造体メンバーへの間接参照
- [ ] メタデータの正確性
- [ ] 型安全性の検証
- [ ] 後方互換性の確認

### Phase 3のテスト
- [ ] ポインタ演算の正確性
- [ ] 配列走査のパフォーマンス
- [ ] 境界外アクセスの検出
- [ ] 複雑なポインタ操作

## マイルストーン

### M1: Phase 1完了（目標: 1週間）
- 配列初期化バグ修正
- アドレス取得機能の完成
- テストケース拡充
- ドキュメント整備
- **成功基準**: 全テストスイート通過 + 新規テスト10件以上追加

### M2: Phase 2完了（目標: 2週間）
- メタデータシステム実装
- 間接参照の修正
- 後方互換性確認
- **成功基準**: 配列/構造体要素への間接参照が動作

### M3: Phase 3完了（目標: 2週間）
- ポインタ演算実装
- 配列走査最適化
- **成功基準**: ポインタ演算を使った配列走査が動作

## リスク管理

### 高リスク項目
1. **メタデータ導入による既存コードの破壊**
   - 軽減策: 段階的移行、後方互換性の維持
   
2. **パフォーマンスの劣化**
   - 軽減策: ベンチマーク測定、最適化パス

3. **メモリリークの可能性**
   - 軽減策: メモリ管理の明確化、RAII原則

### 中リスク項目
4. **型安全性の不完全性**
   - 軽減策: 段階的な型チェック強化

5. **エッジケースの見落とし**
   - 軽減策: 包括的なテストケース

## レビューポイント

### Phase 1のレビュー
- [ ] 配列初期化が全ケースで動作するか
- [ ] 新規テストが既存機能を壊していないか
- [ ] ドキュメントが正確で分かりやすいか

### Phase 2のレビュー
- [ ] メタデータ構造は拡張可能か
- [ ] 後方互換性が保たれているか
- [ ] メモリ管理は適切か

### Phase 3のレビュー
- [ ] ポインタ演算は標準的な動作をするか
- [ ] パフォーマンスは許容範囲か
- [ ] エラーハンドリングは適切か
