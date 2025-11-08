# Cb Sample Programs

このディレクトリには、Cb言語の様々な機能を示すサンプルプログラムが含まれています。
サンプルは目的別にフォルダ分けされています。

## フォルダ構成

### 📁 basic/ - 基本的な言語機能
初心者向けの基本的なプログラム例

- `fibonacci.cb` - フィボナッチ数列の計算
- `fizzbuzz.cb` - FizzBuzz問題
- `array.cb` - 配列の基本操作
- `strlen.cb` - 文字列長の計算

### 📁 async/ - 非同期処理 (v0.12.0)
async/await を使った非同期プログラミング

- `async_basic.cb` - async/awaitの基本的な使い方
  - 順次実行（await使用）
  - 並行実行（awaitなし）
  - 混合パターン
- `async_cooperative.cb` - 協調的マルチタスクの動作例
  - 複数ワーカーの協調動作
  - 長短タスクの混在
  - 逐次 vs 並行の比較
- `async_interface_impl_demo.cb` - asyncとインターフェースの組み合わせ

### 📁 stdlib/ - 標準ライブラリ
標準ライブラリのコンテナとユーティリティの使用例

- `vector_demo.cb` - Vectorコンテナの使用例
- `queue_demo.cb` - Queueコンテナの使用例
- `time_demo.cb` - Time モジュールの使用例（時刻取得、計測、sleep）
- `string_demo.cb` - String モジュールの使用例（文字列操作全般）

### 📁 algorithm/ - アルゴリズム実装
有名なアルゴリズムの実装例

- `dijkstra_struct.cb` - ダイクストラ法による最短経路探索
- `knapsack_dp.cb` - ナップザック問題の動的計画法による解法
- `tsp_bitdp.cb` - 巡回セールスマン問題のビットDP解法

### 📁 data_structures/ - データ構造
データ構造の実装例

- `linked_list_stack.cb` - 連結リストとスタックの実装
- `string_wrapper.cb` - 文字列ラッパー構造体

### 📁 advanced/ - 高度な機能
高度な言語機能のデモ

- `function_pointer_demo.cb` - 関数ポインタの使用例
- `typedef_comprehensive_demo.cb` - typedef の包括的な使用例
- `forward_declaration_demo.cb` - 前方宣言のデモ
- `forward_value_member_demo.cb` - 前方値メンバーのデモ

## サンプルの実行方法

```bash
# 基本的な実行
./main sample/basic/fibonacci.cb

# async/awaitサンプルの実行（時間計測付き）
./main sample/async/async_basic.cb
./main sample/async/async_cooperative.cb

# アルゴリズムサンプル
./main sample/algorithm/dijkstra_struct.cb

# 標準ライブラリサンプル
./main sample/stdlib/vector_demo.cb
```

## 学習推奨順序

### 初級 - 言語の基礎を学ぶ
1. `basic/fibonacci.cb` - 基本的な関数と再帰
2. `basic/fizzbuzz.cb` - 条件分岐とループ
3. `basic/array.cb` - 配列操作
4. `stdlib/vector_demo.cb` - 動的配列の使用

### 中級 - データ構造とアルゴリズム
5. `data_structures/linked_list_stack.cb` - カスタムデータ構造
6. `algorithm/knapsack_dp.cb` - 動的計画法
7. `algorithm/dijkstra_struct.cb` - グラフアルゴリズム

### 上級 - 高度な機能
8. `advanced/typedef_comprehensive_demo.cb` - 型エイリアス
9. `advanced/function_pointer_demo.cb` - 関数ポインタ
10. `async/async_basic.cb` - 非同期処理の基礎
11. `async/async_cooperative.cb` - 協調的マルチタスク

## カテゴリ別クイックリファレンス

### 制御構造を学びたい
- `basic/fizzbuzz.cb` - if/else, for, while
- `basic/fibonacci.cb` - 再帰

### データ構造を学びたい
- `basic/array.cb` - 配列
- `stdlib/vector_demo.cb` - 動的配列
- `stdlib/queue_demo.cb` - キュー
- `data_structures/linked_list_stack.cb` - 連結リスト

### アルゴリズムを学びたい
- `algorithm/knapsack_dp.cb` - 動的計画法
- `algorithm/dijkstra_struct.cb` - グラフ探索
- `algorithm/tsp_bitdp.cb` - ビットDP

### 非同期処理を学びたい
- `async/async_basic.cb` - async/awaitの基本
- `async/async_cooperative.cb` - マルチタスク

### 高度な型システムを学びたい
- `advanced/typedef_comprehensive_demo.cb` - typedef
- `advanced/forward_declaration_demo.cb` - 前方宣言

## テストとの違い

- **サンプル (sample/)**: 機能の使用例を示す教育的なコード
- **テスト (tests/)**: 機能の正確性を検証する自動テスト

サンプルは学習目的で設計されており、実際のユースケースを示しています。
