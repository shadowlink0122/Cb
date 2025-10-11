# デフォルト引数テスト

デフォルト引数機能のテストスイート

## テストケース

### 基本機能

#### test_default_args_basic.cb
基本的なデフォルト引数の動作を検証

**テスト内容**:
- 全ての引数を明示的に指定
- 最後の引数をデフォルト値で使用
- 複数の引数をデフォルト値で使用

**期待される出力**:
```
6
23
31
```

#### test_default_args_types.cb
様々な型のデフォルト引数を検証

**テスト内容**:
- int, string, bool型のデフォルト値
- 全ての引数をデフォルト値で呼び出し
- 部分的にデフォルト値を使用
- 全ての引数を明示的に指定

**期待される出力**:
```
--- All defaults ---
42
hello
1
--- Partial defaults ---
100
hello
1
--- Partial defaults 2 ---
100
world
1
--- All specified ---
200
custom
0
--- Multiply test ---
30
60
40
```

#### test_default_args_const.cb
const変数をデフォルト値として使用

**テスト内容**:
- const変数をデフォルト値に指定
- 様々なパターンで関数呼び出し

**期待される出力**:
```
--- Default window ---
(Window: 800x600)
--- Custom width ---
(Window: 1024x600)
--- Custom width and height ---
(Window: 1024x768)
--- All custom ---
(Full HD: 1920x1080)
--- Compute test ---
50
100
```

### 高度な機能

#### test_default_args_struct.cb
struct型とデフォルト引数の組み合わせ

**テスト内容**:
- structを返す関数でデフォルト引数を使用
- structをパラメータとして受け取る関数でデフォルト引数を使用

**期待される出力**:
```
--- Function returning struct with default args ---
Point: (10, 20)
P2: (100, 20)
P3: (100, 200)
--- Struct parameter with default label ---
Point: (50, 75)
Custom: (50, 75)
```

#### test_default_args_array.cb
配列パラメータとデフォルト引数の組み合わせ

**テスト内容**:
- 配列パラメータを持つ関数でデフォルト引数を使用
- 配列を受け取りながら他のパラメータにデフォルト値を設定

**期待される出力**:
```
--- Sum with default multiplier ---
15
--- Sum with custom multiplier ---
30
--- Print with default prefix ---
Array:
10
20
30
--- Print with custom prefix ---
Values:
10
20
30
```

### エラーケース

#### test_default_args_error1.cb
デフォルト引数の後に非デフォルト引数を配置（コンパイルエラー）

**期待される動作**: パーサーエラーで実行拒否

**エラーメッセージ**:
```
error: Non-default parameter 'b' after default parameter
```

#### test_default_args_error2.cb
必須引数の不足（実行時エラー）

**期待される動作**: 実行時エラーで終了

**エラーメッセージ**:
```
Error: Argument count mismatch for function: add (expected 1 to 2, got 0)
```

## 実装の詳細

### パーサー拡張
- パラメータ宣言で `= 式` を解析
- デフォルト値の位置検証（右側から連続）
- first_default_param_indexの記録

### インタプリタ拡張
- 引数数の検証（必須引数〜全パラメータ）
- デフォルト値の評価と補完
- const修飾の保持

## 制約

1. **右側から連続**: デフォルト引数は右側から連続して指定する必要がある
2. **定数式のみ**: デフォルト値は定数式（リテラル、const変数）のみ
3. **スキップ不可**: 中間の引数をスキップできない
4. **型一致**: デフォルト値の型はパラメータの型と一致する必要がある

## HPP統合テスト

デフォルト引数のテストは統合テストフレームワーク（HPP）に完全統合されています。

**ファイル**: `tests/integration/default_args/test_default_args.hpp`

**統合テスト結果**:
- 7テストケース
- 57アサーション
- 全て成功 ✅

**実行方法**:
```bash
make integration-test
```

**統合されたテスト**:
1. Basic default arguments functionality (8 assertions)
2. Default arguments with various types (10 assertions)
3. const variables as default values (6 assertions)
4. struct types with default arguments (5 assertions)
5. Array parameters with default arguments (12 assertions)
6. Error detection: non-default parameter after default (4 assertions)
7. Error detection: missing required argument (4 assertions)

## 今後の拡張

- [ ] より複雑な定数式のサポート
- [ ] 名前付き引数との組み合わせ
- [ ] コンストラクタでのデフォルト引数サポート（v0.10.0で予定）
