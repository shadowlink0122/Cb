# トラブルシューティング - Docker ビルドエラー

## エラー: "Unable to locate package libdl-dev"

### 症状
```
E: Unable to locate package libdl-dev
```

### 原因
Ubuntu 22.04では`libdl-dev`パッケージが存在しません。`libdl`は`libc6-dev`（`build-essential`に含まれる）に統合されています。

### 解決済み
Dockerfileから`libdl-dev`を削除しました。

## エラー: アーキテクチャ不一致

### 症状
ARM64 (Apple Silicon)マシンでビルドすると、x86_64バイナリのダウンロードに失敗する。

### 解決済み
Dockerfileを修正し、自動的にアーキテクチャを検出してダウンロードするようにしました：
- Go: `linux-amd64` または `linux-arm64`
- Zig: `linux-x86_64` または `linux-aarch64`

## ビルド時間

初回ビルドには**10-15分**かかります（正常です）：
- Rustのインストール: 3-5分
- Goのダウンロード: 1-2分
- Zigのダウンロード: 1-2分

## ビルド成功の確認

```bash
# Dockerイメージの確認
docker images | grep cb-ffi-test

# 出力例:
# cb-ffi-test   latest   756fe71b8bb9   1 minute ago   2.75GB
```

## 次のステップ

ビルドが成功したら：

```bash
# テストを実行
make test-c      # Cライブラリのテスト
make test-cpp    # C++ライブラリのテスト
make test        # すべてのテスト
```
