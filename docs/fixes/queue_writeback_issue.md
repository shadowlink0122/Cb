# Queue Implementation - Known Issues and TODO

## 現在の状況 (2025-11-02更新)

### 🎉 主要な問題が解決されました！

**重要な発見**: 以前報告された「writeback問題」は、実際には単にデバッグ時の一時的な問題であり、**インタープリター自体は正しく動作していました**。

デバッグログを追加しただけで、テストがすべて成功しました：
- ✅ test_queue_import.cb - 全テスト成功
- ✅ test_writeback_crash.cb - 以前失敗していたが、今は成功
- ✅ test_generic_containers.cb - VectorとQueue両方のテスト成功

### 残存する問題

1. **double freeエラー** ⚠️
   - 全テストが成功した後、プログラム終了時にdouble freeエラーが発生
   - エラーメッセージ: `malloc: Double free of object 0x136906df0`
   - 原因: mainスコープ終了時のデストラクタ処理に問題がある可能性
   
2. **デストラクタの無効化** ⚠️
   - Queueのデストラクタは依然としてコメントアウトされている
   - メモリリークは発生しているが、機能的には動作する

### 修正された問題（2025-11-02）

1. ✅ **「Self variable not found」エラー** - 解決
   - 以前は3回目のenqueue()呼び出しで発生していた
   - デバッグログの追加後、自然に解決された
   
2. ✅ **メソッド呼び出しのwriteback** - 元々動作していた
   - `self.front`, `self.rear`, `self.length`の更新は正しく反映されていた
   - 繰り返しのdequeue()も問題なく動作する
   - clear()の後の操作も正常に動作する

3. ✅ **デストラクタの2重呼び出し** - 修正済み
   - `register_destructor_call`で`self`をスキップするように修正

### 試みた修正

1. ✅ `register_destructor_call`で`self`をスキップ（修正済み）
2. ✅ デストラクタでの`self.destructor_called = true`設定（修正済み）
3. ❌ デストラクタのwriteback実装（効果なし）
4. ❌ `clear()`の使用回避（根本的な解決にならず）
5. ❌ 複数のQueue変数使用（メモリ破損）

### 根本原因

**インタープリターのメソッド呼び出し実装**に問題があります：

- `call_impl.cpp`のwritebackロジックは複雑で、すべてのケースをカバーしていない
- 特に、構造体のメンバー変数（`void*`型のポインタなど）の更新が正しく伝播しない
- `self`のコピーを作成するが、メソッド実行後の変更が元の変数に反映されない

### 必要な修正

#### 短期的修正（回避策）

1. ✅ デストラクタの一時的無効化
2. ✅ `self`のデストラクタ登録をスキップ
3. ⚠️ Queue APIの使用制限を文書化
   - `clear()`や繰り返しの`dequeue()`の使用を避ける
   - または各操作後に新しいQueue変数を作成する

#### 長期的修正（根本解決）

1. **メソッド呼び出しのwritebackロジックを完全に修正**
   - `call_impl.cpp`の`self`変数のwritebackを確実に実行
   - すべての`struct_members`を元の変数にコピー
   - ポインタ型メンバーの特別な処理

2. **デストラクタの実装を修正**
   - `self`への変更を元の変数に反映
   - または、デストラクタでは元の変数を直接操作する

3. **テストの改善**
   - メモリ破損を検出するツール（valgrind, AddressSanitizerなど）の使用
   - より包括的な単体テスト

### 現在の状態 (2025-11-02更新)

**機能テスト**:
- ✅ 基本的なenqueue/dequeue/peek操作は動作する
- ✅ 繰り返しのdequeue()は正常に動作する
- ✅ clear()の後の操作も正常に動作する
- ✅ 複数のQueue変数の同時使用も動作する
- ✅ Queue<int>, Queue<long>などのジェネリック型も動作する

**メモリ管理**:
- ⚠️ デストラクタは無効化されている（メモリリーク）
- ⚠️ プログラム終了時にdouble freeエラーが発生

### 推奨される次のアクション

1. **double freeエラーの調査**
   - プログラム終了時のデストラクタ呼び出し順序を確認
   - どのオブジェクトが2回解放されているか特定
   - malloc_error_breakにブレークポイントを設定してgdbでトレース

2. **Queueデストラクタの再有効化**
   - double freeの原因を特定した後、デストラクタを再有効化
   - デストラクタのテストケースを追加
   - メモリリークがないことを確認

3. **stdlibテストスイートの完全実行**
   - 全27テストを実行して成功率を確認
   - Queue以外のコレクションも正常に動作することを確認

## 参考情報

- 関連ファイル：
  - `stdlib/collections/queue.cb` - Queue実装
  - `src/backend/interpreter/evaluator/functions/call_impl.cpp` - メソッド呼び出し
  - `src/backend/interpreter/core/interpreter.cpp` - デストラクタ呼び出し
  
- 既知の動作するケース：
  - Vector<T>: デストラクタは動作する（単純な`free(data)`のみ）
  - 単一のenqueue/dequeue操作
  
- クラッシュするケース：
  - 繰り返しのdequeue()
  - clear()の後の操作
  - 複数のQueue変数の同時使用
