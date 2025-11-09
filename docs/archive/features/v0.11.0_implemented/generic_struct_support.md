# ジェネリックコンテナでの構造体サポート実装レポート

**日付**: 2025年10月30日  
**ブランチ**: feature/trait-allocator  
**実装者**: AI Assistant with shadowlink

## 概要

Cbプログラミング言語のジェネリックコンテナ（`Vector<T>`、`Queue<T>`、`Box<T>`など）で構造体型をサポートするための実装を行いました。

## 実装内容

### 1. ジェネリックimplでの構造体パラメータ対応

**修正ファイル**:
- `src/backend/interpreter/evaluator/core/evaluator.cpp`
- `src/backend/interpreter/executors/statement_executor.cpp`
- `src/backend/interpreter/evaluator/functions/call_impl.cpp`
- `src/backend/interpreter/managers/structs/operations.cpp`
- `src/backend/interpreter/handlers/control/return.cpp` (前回セッション)

**主な変更点**:

1. **evaluator.cpp (Line 1170-1201)**
   - `AST_IDENTIFIER`での構造体変数評価を改善
   - `ReturnException`を避けて`TypedValue`を直接返すように修正
   ```cpp
   // 構造体変数の場合、TypedValueとして返す（ReturnExceptionを避ける）
   if (var->type == TYPE_STRUCT) {
       return TypedValue(*var, InferredType(TYPE_STRUCT, var->struct_type_name));
   }
   ```

2. **statement_executor.cpp (Line 709-779)**
   - `execute_self_member_assignment`で構造体の代入をサポート
   - `AST_IDENTIFIER`または`AST_VARIABLE`で構造体を検出し、完全なコピーを実行
   ```cpp
   if (source_var && source_var->type == TYPE_STRUCT) {
       // 構造体データを完全にコピー
       *self_member = *source_var;
       // ダイレクトアクセス変数も更新
       interpreter_.sync_direct_access_from_struct_value("self." + member_name, *self_member);
   }
   ```

3. **call_impl.cpp (Line 3055-3170, 3336-3515)**
   - メソッド終了時の`self`書き戻しで構造体メンバーの`struct_members`もコピー
   - 通常終了とreturn文の両方で対応
   ```cpp
   // 構造体メンバーの場合、struct_membersもコピー
   if (self_member_var.type == TYPE_STRUCT && 
       !self_member_var.struct_members.empty()) {
       receiver_member_var->struct_members = self_member_var.struct_members;
   }
   ```

4. **operations.cpp (Line 571-593)**
   - `sync_individual_member_from_struct`で構造体の`struct_members`をコピー
   - ネストされた構造体メンバーも再帰的に同期
   ```cpp
   // ネストされた構造体のメンバーも再帰的に同期
   for (const auto &nested_member : member_value.struct_members) {
       // 個別変数を更新
   }
   ```

## テスト結果

### ✅ 動作確認済み

1. **Box<Point>テスト** (`test_box_point.cb`)
   ```
   ✅ Box<Point> test PASSED!
   ```
   - 構造体の代入（`set(Point p)`）が正常動作
   - 構造体の取得（`Point get()`）が正常動作
   - メンバーアクセス（`b.value.x`, `b.value.y`）が正常動作

2. **Integration tests**
   ```
   Total:  3516
   Passed: 3516
   Failed: 0
   🎉 ALL TESTS PASSED! 🎉
   ```
   - 全3516テストが合格
   - 既存機能に影響なし

3. **基本的な構造体操作**
   - `Box<T>.set(T value)` - ✅ 動作
   - `Box<T>.get()` -> T - ✅ 動作
   - `self.value = v` (構造体の代入) - ✅ 動作

### ⚠️ 制限事項

1. **配列ベースのジェネリック関数**
   - `Vector<T>.get(index)` -> T - ❌ 構造体を返せない
   - `Queue<T>.dequeue()` -> T - ❌ 構造体を返せない
   - **原因**: `array_get()`がintのみをサポート
   - **回避策**: `get_ptr(index)`を使ってポインタ経由でアクセス

2. **構造体返却時の型推論**
   - `array_get()`内で型パラメータ`T`が構造体かどうかを判定する機能がない
   - ジェネリックコンテキストから型情報を取得するAPIが不足

## サポート状況マトリクス

| コンテナ | push/enqueue | get/dequeue | set | 備考 |
|---------|-------------|-------------|-----|------|
| Box<T> | ✅ | ✅ | ✅ | 完全サポート |
| Vector<T> | ✅ | ⚠️ | ✅ | get()は制限あり、get_ptr()も制限あり |
| Queue<T> | ✅ | ⚠️ | - | dequeue()は制限あり |

**注意**: `get_ptr()`は動作し、ポインタを返します。ただし、返されたポインタに対するアロー演算子`->`は現時点ではサポートされていません。デリファレンス演算子`(*ptr).x`を使用してください。

## 使用例

### Box<Point> (完全サポート)

```cb
struct Point {
    int x;
    int y;
};

fn main() {
    Box<Point> b;
    b.value.x = 0;
    b.value.y = 0;
    
    Point p;
    p.x = 100;
    p.y = 200;
    
    b.set(p);  // ✅ 動作
    
    println("b.value.x = ", b.value.x);  // ✅ 動作
    println("b.value.y = ", b.value.y);  // ✅ 動作
    
    Point result = b.get();  // ✅ 動作
    println("result.x = ", result.x, ", result.y = ", result.y);
}
```

### Vector<Point> (部分サポート)

```cb
fn main() {
    Vector<Point> vec;
    vec.init(5);
    
    Point p1;
    p1.x = 10;
    p1.y = 20;
    
    vec.push(p1);  // ✅ 動作
    
    // ❌ これは動作しない（array_getが構造体未対応）
    // Point retrieved = vec.get(0);
    
    // ✅ 回避策1: get_ptr()を使用してデリファレンス
    void* ptr = vec.get_ptr(0);
    Point* p_ptr = (Point*)ptr;
    println("(*p_ptr).x = ", (*p_ptr).x);  // ✅ デリファレンス演算子は動作
    
    // ❌ アロー演算子は未サポート
    // println("p_ptr->x = ", p_ptr->x);  // ❌ 生メモリポインタへのアロー演算子は動作しない
}
```

## 今後の改善点

### 優先度: 高

1. **array_get/array_setの構造体対応**
   - ジェネリック型情報へのアクセスAPIを追加
   - `array_get()`で構造体の場合は`ReturnException`をスロー
   - `array_set()`で構造体の場合は`TypedValue`から取得

### 優先度: 中

2. **配列アクセス演算子のサポート**
   - `vec[i]` -> T の構文サポート（将来的な課題）
   - オペレーターオーバーロードの実装が必要

### 優先度: 低

3. **パフォーマンス最適化**
   - 構造体のコピー最適化
   - move semanticsのサポート

## 技術的な課題

### 課題1: ジェネリック型情報へのアクセス

**問題**:
`array_get()`や`array_set()`の実装内で、現在のimplコンテキストの型パラメータ`T`が何であるかを判定する方法がない。

**必要なAPI**:
```cpp
// implコンテキスト内かどうかを判定
bool interpreter_.is_in_impl_context();

// 現在の型パラメータのバインディングを取得
std::map<std::string, std::string> interpreter_.get_current_type_bindings();

// 構造体のサイズを取得
size_t interpreter_.get_struct_size(const std::string& type_name);

// 構造体メンバーのオフセットを取得
size_t interpreter_.get_member_offset(const std::string& type_name, 
                                      const std::string& member_name);
```

### 課題2: 構造体のローカル変数宣言

**問題**:
Cbでは`T result;`のような未初期化変数宣言ができないため、`memcpy`の結果を受け取る変数を作成できない。

**現在の回避策**:
- `ReturnException`を使って構造体を返す
- `get_ptr()`を使ってポインタ経由でアクセス

**理想的な解決策**:
```cb
T get(int index) {
    // これができれば理想的
    T result;  // デフォルト初期化
    memcpy(&result, src, sizeof(T));
    return result;
}
```

## 修正履歴

### 2025年10月30日 - get_ptr()のセグフォルト修正

**問題**: `Vector<T>.get_ptr()`を呼び出すとセグメンテーションフォルトが発生

**原因**:
1. `sizeof(T)`がimplメソッド内で呼び出されたとき、`is_method_call`条件によってスキップされていた
2. `void* ptr = self.data + offset;`のような`void*`型の一時変数への代入でセグフォルトが発生

**修正**:
1. `call_impl.cpp` Line 1215: `sizeof`の条件から`!is_method_call`を削除
   ```cpp
   // 修正前
   if (node->name == "sizeof" && !is_method_call) {
   
   // 修正後
   if (node->name == "sizeof") {
   ```

2. `stdlib/collections/vector.cb`: `get_ptr()`の実装を一時変数なしに変更
   ```rust
   // 修正前
   void* ptr = self.data + (index * element_size);
   return ptr;
   
   // 修正後
   return self.data + (index * element_size);
   ```

**結果**: 
- ✅ `get_ptr()`のセグフォルトは解消されました
- ✅ 返されたポインタに対するデリファレンス演算子`(*ptr).x`は動作します
- ❌ 返されたポインタに対するアロー演算子`ptr->x`は現時点では動作しません（生メモリポインタへのアロー演算子は未サポート）

## まとめ

### 達成したこと

✅ ジェネリックimplでの構造体パラメータ渡しが機能  
✅ 構造体メンバーへの代入が機能  
✅ `Box<T>`で構造体が完全にサポート  
✅ メソッド実行後の構造体の正しい書き戻し  
✅ ネストされた構造体メンバーの同期  
✅ 全3516テストが引き続き合格  
✅ `sizeof(T)`がimplメソッド内で正しく動作  
✅ `Vector<T>.get_ptr()`がセグフォルトせずに動作  

### 残された課題

⏸️ `Vector<T>.get()`などの配列ベース関数で構造体を返す処理  
⏸️ `array_get`/`array_set`の構造体対応  
⏸️ 生メモリポインタへのアロー演算子`->`のサポート（`ptr->x`形式）  
⏸️ 配列アクセス演算子`vec[i]`のサポート（将来的な課題）  

**技術的な理由**:
- アロー演算子`ptr->x`が動作しないのは、`malloc()`で確保された生メモリポインタが`Variable*`ではなく、`evaluate_arrow_access`が構造体型情報を持たないためです
- 回避策として、デリファレンス演算子`(*ptr).x`を使用することができます

### 評価

構造体サポートの**基本的な部分は完成**しました。`Box<T>`のような単一要素コンテナは完全に動作し、`Vector<T>`や`Queue<T>`も`push/enqueue`は機能します。

`get_ptr()`のセグフォルトは修正されましたが、配列操作との完全な統合にはまだ技術的な課題が残っています。

真のジェネリックコンテナへの道は着実に進んでいます！🚀

## 関連ファイル

- 実装: `src/backend/interpreter/evaluator/core/evaluator.cpp`
- 実装: `src/backend/interpreter/executors/statement_executor.cpp`
- 実装: `src/backend/interpreter/evaluator/functions/call_impl.cpp`
- 実装: `src/backend/interpreter/managers/structs/operations.cpp`
- テスト: `test_box_point.cb` ✅
- テスト: `test_struct_containers.cb` ⚠️
- テスト: `test_struct_containers_ref.cb` ⚠️

---

**次のステップ**: ジェネリック型情報へのアクセスAPIを設計・実装し、`array_get`/`array_set`の構造体対応を完成させる。
