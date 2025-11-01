# Queue Implementation - Known Issues and TODO

## 現在の状況 (2025-11-01)

### 発見された重大なバグ

1. **メソッド呼び出しのwriteback問題**
   - `dequeue()`, `clear()`などのメソッドが`self`のメンバー（`front`, `rear`, `length`）を更新する
   - しかし、これらの更新が**元の変数に書き戻されない**
   - 結果：2回目以降のメソッド呼び出しで、古いポインタにアクセスしてクラッシュ

2. **デストラクタの実装問題**
   - デストラクタ内で`self`への変更が元の変数に反映されない
   - デストラクタが2重に呼ばれる問題（部分的に修正済み）
   - 現在デストラクタは無効化されている（メモリリーク発生中）

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

### 現在の状態

- ✅ 基本的なenqueue/dequeue/peek操作は動作する（1回のみ）
- ❌ 繰り返しのdequeue()はクラッシュする
- ❌ clear()の後の操作はクラッシュする
- ❌ デストラクタは無効化されている（メモリリーク）
- ❌ 複数のQueue変数を使用するとメモリ破損

### 推奨される次のアクション

1. **lldb/gdbでのデバッグ**
   - `call_impl.cpp`のwritebackロジックを詳細にトレース
   - `self`のアドレスと元の変数のアドレスを確認
   - `struct_members`のコピー処理を確認

2. **最小限の再現ケースを作成**
   ```cb
   Queue<int> q;
   q.enqueue(1);
   q.dequeue(); // 1回目: OK
   q.enqueue(2);
   q.dequeue(); // 2回目: CRASH
   ```

3. **インタープリターコードの詳細レビュー**
   - `call_impl.cpp`の行3195-3300あたり（writebackロジック）
   - `interpreter.cpp`の`call_destructor`関数
   - `member_assignment.cpp`の構造体メンバー代入処理

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
