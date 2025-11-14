# FFI Test Quick Start

## すぐに始める

```bash
cd tests/ffi
make build    # Dockerイメージをビルド（初回のみ、10-15分かかります）
make test     # すべてのFFIテストを実行
```

## 個別テスト

```bash
make test-c      # Cライブラリのみ
make test-cpp    # C++ライブラリのみ
make test-rust   # Rustライブラリのみ
make test-go     # Goライブラリのみ
make test-zig    # Zigライブラリのみ
```

## デバッグ

```bash
make shell       # Dockerコンテナのシェルに入る
```

コンテナ内で：
```bash
cd /cb
make             # Cbインタプリタをビルド

cd /cb/tests/ffi/libs/c
make             # Cライブラリをビルド
cp libclib.so /cb/stdlib/foreign/

cd /cb/tests/ffi
/cb/main tests/c/basic_test.cb  # 手動でテスト実行
```

## 標準ライブラリのラッパーテスト

各言語の標準ライブラリもFFI経由で使えることを確認：

- **C**: `tests/c/stdlib_test.cb` → sin, cos, abs, ceil, floor
- **Go**: `tests/go/concurrent_test.cb` → math.Sin, math.Cos
- **Zig**: `tests/zig/math_test.cb` → std.math.pow, std.math.pi

## クリーンアップ

```bash
make clean       # Dockerイメージとコンテナを削除
```
