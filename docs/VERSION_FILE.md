# .cbversionファイルについて

## 概要

`.cbversion`ファイルは、Cb言語プロジェクト全体のバージョン管理を行うためのマスターバージョンファイルです。

## ファイル形式

```
0.13.0
```

- **場所**: プロジェクトルート (`Cb/.cbversion`)
- **形式**: `x.y.z` (セマンティックバージョニング)
- **例**: `0.13.0`, `1.0.0`, `2.5.3`

## なぜ`.cbversion`という名前なのか

### 問題

当初は`VERSION`という名前でしたが、以下の問題が発生しました：

```bash
$ make
g++ -Wall -g -std=c++17 -I. -Isrc ...
./version:1:1: error: expected unqualified-id
0.13.0
^
```

Makefileの`-I.`オプション（カレントディレクトリをインクルードパス追加）により、C++コンパイラが`VERSION`ファイルをヘッダーファイルとして誤って読み込もうとしました（macOSでは大文字小文字を区別しないため、`<version>`と`VERSION`が衝突）。

### 解決策

- ファイル名を`.cbversion`に変更
- ドットで始まるファイル名は通常のインクルードパスから除外される
- Cb固有のファイルであることが明確

## 使用方法

### バージョンの確認

```bash
cat .cbversion
```

### バージョンの更新

```bash
# 新しいバージョンを設定
echo "0.14.0" > .cbversion

# VSCode拡張機能のバージョンを自動同期
make update-extension-version

# 確認
make verify-extension-version
```

## 連携

`.cbversion`ファイルは以下のコンポーネントと連携します：

1. **VSCode拡張機能**
   - `vscode-extension/package.json`のバージョンと自動同期
   - `make update-extension-version`で同期
   - `make build-extension`で自動検証

2. **ビルドシステム**
   - Makefileから参照可能
   - リリースタグの生成に使用

3. **CI/CD**
   - バージョン一貫性チェックに使用
   - 自動リリースビルドのトリガー

## Git管理

`.cbversion`ファイルは**必ずGitで管理**してください：

```bash
# 追加
git add .cbversion

# コミット
git commit -m "Bump version to 0.14.0"

# タグ作成
git tag v0.14.0

# プッシュ
git push origin main --tags
```

## セマンティックバージョニング

`.cbversion`はセマンティックバージョニングに従います：

```
MAJOR.MINOR.PATCH
  |     |     |
  |     |     +-- バグ修正、小さな変更
  |     +-------- 新機能追加（後方互換性あり）
  +-------------- 破壊的変更（後方互換性なし）
```

### 例

- `0.13.0` → `0.13.1`: バグ修正、小さな改善
- `0.13.1` → `0.14.0`: 新機能追加（FFI、プリプロセッサなど）
- `0.14.0` → `1.0.0`: メジャーリリース、言語仕様の大幅変更

## トラブルシューティング

### ファイルが見つからない

```bash
# .cbversionファイルを作成
echo "0.13.0" > .cbversion
```

### VSCode拡張機能とのバージョン不一致

```bash
# 同期
make update-extension-version

# 確認
make verify-extension-version
```

### フォーマットエラー

正しい形式：
- ✅ `0.13.0`
- ✅ `1.0.0`
- ✅ `10.5.23`

間違った形式：
- ❌ `0.13` (PATCHが不足)
- ❌ `v0.13.0` (vプレフィックス不要)
- ❌ `0.13.0-beta` (サフィックス不可)

## 関連ドキュメント

- [VSCode拡張機能のバージョン管理](../vscode-extension/VERSION_MANAGEMENT.md)
- [セマンティックバージョニング](https://semver.org/lang/ja/)
