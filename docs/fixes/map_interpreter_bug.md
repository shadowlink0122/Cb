# Map Implementation Known Issues

## インタプリタバグとの相互作用

### 問題の要約
`Map<K, V>`の実装において、2回目の`insert`呼び出し時にheap-use-after-freeエラーが発生します。
これはCbインタプリタ自身のメモリ管理バグにより引き起こされています。

### 技術的詳細

#### 発見された問題
1. **ジェネリック型パラメータマッチング** (解決済み)
   - `interfaces.cpp`で`Map<K, V>`のような複数型パラメータのマッチングが失敗
   - 修正: 型パラメータ数のチェックを追加

2. **Builtin関数呼び出し** (解決済み)
   - ジェネリックコンテキストから`malloc`を呼ぶと`func`がnullptr
   - 修正: `call_impl.cpp`で早期処理を追加

3. **メモリレイアウト問題** (解決済み)
   - Map実装が4バイト整数を想定、実際は8バイトで格納される
   - 修正: 全オフセットを固定8バイトアライメントに変更

4. **構造体メンバー代入時のheap-use-after-free** (未解決・インタプリタバグ)
   - AddressSanitizerが検出した問題
   - `simple_assignment.cpp:219`でWrite発生
   - `manager.cpp:1339`で既に解放済みのメモリにアクセス
   - 構造体の`void*`型メンバー（後に`long`に変更）への代入時に発生
   - VectorやQueueでは発生しない（メンバー構成の違い？）

#### ASANスタックトレース（抜粋）
```
ERROR: AddressSanitizer: heap-use-after-free
WRITE of size 8 at simple_assignment.cpp:219
freed by thread T0 here:
    #0 wrap__ZdlPv in libclang_rt.asan
    #1 ExpressionService::evaluate_safe
    #2 TypeManager::check_type_range (manager.cpp:141)
    #3 VariableManager::assign_variable (manager.cpp:1339)
```

### 試みた解決策

1. ✗ Map構造体を簡素化（`root`ポインタのみ保持）
2. ✗ `void*`から`long`への型変更
3. ✗ AVLバランシングの無効化
4. ✗ デバッグ出力の削除（コンストラクタ内のprintlnがクラッシュ原因と誤認）

### 推測される原因

インタプリタが以下のいずれかで問題を起こしている：

1. **構造体のコピー/ムーブセマンティクス**
   - ジェネリック関数からのreturn時に構造体がコピーされる
   - 元の`self`ポインタが無効になる
   - 後続の代入が解放済みメモリにアクセス

2. **ジェネリック型解決時のメモリ管理**
   - `Map<int, int>`のインスタンス化時に一時オブジェクトが作成される
   - スコープ終了時に早期解放される
   - その後の操作が dangling reference を使う

3. **メソッド呼び出し時の`self`ポインタ管理**
   - `self.root = self.insert_to_node(...)`の評価順序
   - 右辺の評価中に左辺の`self`が無効化される

### 回避策の可能性

- グローバル関数として実装（メソッドを使わない）
- 単純な二分探索木（AVLなし、再帰なし、イテレーティブ実装）
- インタプリタのバグ修正を待つ

### 関連ファイル

- `stdlib/collections/map.cb` - Map実装
- `src/backend/interpreter/managers/types/interfaces.cpp` - 型マッチング
- `src/backend/interpreter/evaluator/functions/call_impl.cpp` - 関数呼び出し
- `src/backend/interpreter/executors/assignments/simple_assignment.cpp:219` - クラッシュ箇所
- `src/backend/interpreter/managers/variables/manager.cpp:1339` - メモリ解放箇所

### 次のステップ

1. インタプリタのメモリ管理ロジックをデバッグ
   - `manager.cpp:1339`での解放タイミングを確認
   - `simple_assignment.cpp:219`での代入対象を確認
   - なぜVectorでは発生しないのか調査

2. 別アプローチの検討
   - C++ std::mapラッパーをbuiltin関数として追加
   - 単純な線形探索版Mapを暫定実装
   - インタプリタバグの最小再現ケースを作成

## まとめ

`Map<K, V>`実装自体は理論的に正しいが、Cbインタプリタの構造体メンバー代入処理に
潜在的なバグがあり、2回目の`insert`呼び出し時にクラッシュする。
これはインタプリタ側の修正が必要な問題である。

日付: 2025-01-XX  
バージョン: Cb v0.11.0
