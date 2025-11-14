# v0.18.0 エコシステムとパッケージ管理

**バージョン**: v0.18.0
**目標**: パッケージマネージャーとエコシステムの構築
**期間**: 2-3ヶ月
**作成日**: 2025-11-14

---

## 概要

v0.18.0では、Cb言語のエコシステムを確立し、パッケージ管理システムを実装します。これにより、開発者がライブラリを簡単に共有・利用できるようになります。

### 主要な目標

1. **パッケージマネージャー** (`cbpkg`) の実装
2. **依存関係管理**システムの構築
3. **バージョン管理**とセマンティックバージョニング
4. **パッケージレジストリ**の構築
5. **ビルドシステム**の統合

---

## v0.18.0の主要な変更点

### 1. パッケージマネージャー（cbpkg）

Cb言語のパッケージマネージャー。Rust の Cargo や JavaScript の npm に相当します。

```bash
# プロジェクトの作成
cbpkg new my-project
cbpkg init

# 依存関係の追加
cbpkg add http-client
cbpkg add json-parser@1.2.3

# ビルドと実行
cbpkg build
cbpkg run

# テストの実行
cbpkg test

# パッケージの公開
cbpkg publish
```

### 2. プロジェクト設定ファイル（cb.toml）

```toml
[package]
name = "my-project"
version = "0.1.0"
authors = ["Your Name <you@example.com>"]
edition = "2025"
license = "MIT"
description = "A sample Cb project"

[dependencies]
http-client = "1.0.0"
json-parser = "1.2.3"

[dev-dependencies]
test-framework = "0.5.0"

[build]
target = "x86_64-unknown-linux-gnu"
backend = "native"

[features]
default = ["std"]
std = []
no_std = []
```

### 3. 依存関係解決

```
my-project (0.1.0)
├── http-client (1.0.0)
│   ├── tcp-socket (2.3.1)
│   └── tls (1.5.0)
│       └── crypto (3.2.0)
└── json-parser (1.2.3)
    └── string-utils (0.8.0)
```

### 4. パッケージレジストリ

中央パッケージレジストリ（https://registry.cb-lang.org）：

- パッケージの検索
- バージョン履歴
- ドキュメント
- ダウンロード統計

### 5. ビルドシステム統合

```bash
# 複数バックエンド対応
cbpkg build --backend=native
cbpkg build --backend=wasm
cbpkg build --backend=typescript

# クロスコンパイル
cbpkg build --target=arm-none-eabi
cbpkg build --target=wasm32-unknown-unknown
```

---

## アーキテクチャ

### パッケージマネージャーの構成

```
cbpkg/
├── src/
│   ├── main.cpp              # エントリーポイント
│   ├── commands/             # コマンド実装
│   │   ├── new.cpp
│   │   ├── add.cpp
│   │   ├── build.cpp
│   │   ├── run.cpp
│   │   └── publish.cpp
│   ├── resolver/             # 依存関係解決
│   │   ├── version.cpp
│   │   ├── dependency.cpp
│   │   └── graph.cpp
│   ├── registry/             # レジストリクライアント
│   │   ├── client.cpp
│   │   └── cache.cpp
│   ├── manifest/             # cb.toml解析
│   │   ├── parser.cpp
│   │   └── validator.cpp
│   └── build/                # ビルドシステム
│       ├── builder.cpp
│       └── cache.cpp
└── tests/
```

---

## 機能詳細

### 1. プロジェクト管理

#### プロジェクトの作成

```bash
$ cbpkg new my-app
     Created binary (application) `my-app` package

$ tree my-app/
my-app/
├── cb.toml
├── src/
│   └── main.cb
└── tests/
    └── test_main.cb
```

生成される `cb.toml`:

```toml
[package]
name = "my-app"
version = "0.1.0"
authors = [""]
edition = "2025"

[dependencies]
```

生成される `src/main.cb`:

```cb
fn main() {
    println("Hello, World!");
}
```

#### ライブラリプロジェクトの作成

```bash
$ cbpkg new my-lib --lib
     Created library `my-lib` package

$ tree my-lib/
my-lib/
├── cb.toml
├── src/
│   └── lib.cb
└── tests/
    └── test_lib.cb
```

### 2. 依存関係管理

#### 依存関係の追加

```bash
$ cbpkg add http-client
    Updating registry
   Adding http-client v1.0.0 to dependencies
```

`cb.toml` に追加される：

```toml
[dependencies]
http-client = "1.0.0"
```

#### バージョン指定

```bash
# 特定のバージョン
cbpkg add json@1.2.3

# バージョン範囲
cbpkg add "crypto@^2.0.0"    # 2.0.0 <= version < 3.0.0
cbpkg add "utils@~1.2.0"     # 1.2.0 <= version < 1.3.0
cbpkg add "logger@>=0.5.0"   # 0.5.0以上
```

#### Gitリポジトリから追加

```bash
cbpkg add my-lib --git https://github.com/user/my-lib
cbpkg add my-lib --git https://github.com/user/my-lib --branch develop
cbpkg add my-lib --git https://github.com/user/my-lib --tag v1.0.0
```

`cb.toml`:

```toml
[dependencies]
my-lib = { git = "https://github.com/user/my-lib", branch = "develop" }
```

#### ローカルパスから追加

```bash
cbpkg add my-local-lib --path ../my-local-lib
```

```toml
[dependencies]
my-local-lib = { path = "../my-local-lib" }
```

### 3. ビルドシステム

#### ビルド

```bash
$ cbpkg build
   Compiling json-parser v1.2.3
   Compiling http-client v1.0.0
   Compiling my-app v0.1.0
    Finished release [optimized] target in 2.34s
```

#### デバッグビルド

```bash
$ cbpkg build --debug
    Finished debug [unoptimized + debuginfo] target in 0.82s
```

#### リリースビルド

```bash
$ cbpkg build --release
    Finished release [optimized] target in 3.15s
```

#### クロスコンパイル

```bash
$ cbpkg build --target=arm-none-eabi
   Compiling for arm-none-eabi
    Finished release target in 2.89s
```

#### 複数バックエンド

```bash
$ cbpkg build --backend=wasm
   Compiling to WebAssembly
    Finished wasm32 target in 1.45s
```

### 4. テストシステム

#### テストの実行

```bash
$ cbpkg test
   Compiling test dependencies
   Compiling my-app v0.1.0 (tests)
     Running 5 tests
test test_add ... ok
test test_subtract ... ok
test test_multiply ... ok
test test_divide ... ok
test test_edge_cases ... ok

test result: ok. 5 passed; 0 failed; 0 ignored
```

#### 特定のテストのみ実行

```bash
$ cbpkg test test_add
     Running 1 test
test test_add ... ok
```

### 5. パッケージの公開

#### ログイン

```bash
$ cbpkg login
Please visit https://registry.cb-lang.org/login
Enter your token: **********************
     Logged in successfully
```

#### 公開

```bash
$ cbpkg publish
   Packaging my-lib v0.1.0
   Verifying my-lib v0.1.0
   Uploading my-lib v0.1.0
  Published my-lib v0.1.0 to registry
```

#### バージョンの更新

```bash
$ cbpkg version patch   # 0.1.0 -> 0.1.1
$ cbpkg version minor   # 0.1.1 -> 0.2.0
$ cbpkg version major   # 0.2.0 -> 1.0.0
```

---

## セマンティックバージョニング

### バージョン形式

```
major.minor.patch[-prerelease][+buildmetadata]
```

例：
- `1.0.0` - 安定版
- `1.2.3` - マイナーアップデート
- `2.0.0-alpha.1` - プレリリース
- `1.0.0+20250114` - ビルドメタデータ付き

### バージョン範囲指定

```toml
[dependencies]
# 完全一致
exact = "1.2.3"

# キャレット（互換性のある更新）
caret = "^1.2.3"    # 1.2.3 <= version < 2.0.0

# チルダ（パッチレベルの更新）
tilde = "~1.2.3"    # 1.2.3 <= version < 1.3.0

# 比較演算子
greater = ">=1.2.0"
less = "<2.0.0"
range = ">=1.2.0, <2.0.0"

# ワイルドカード
wildcard = "1.2.*"  # 1.2.x
any = "*"           # 任意のバージョン
```

---

## 依存関係解決アルゴリズム

### 1. 依存関係グラフの構築

```
A (1.0.0)
├─ B (^2.0.0)
│  ├─ C (^1.5.0)
│  └─ D (^3.0.0)
└─ C (^1.0.0)
```

### 2. バージョン解決

利用可能なバージョン:
- B: [2.0.0, 2.1.0, 2.2.0]
- C: [1.0.0, 1.5.0, 1.6.0, 2.0.0]
- D: [3.0.0, 3.1.0]

解決結果:
- B: 2.2.0 (最新)
- C: 1.6.0 (1.5.0 <= version < 2.0.0 の最新)
- D: 3.1.0 (最新)

### 3. 競合解決

競合が発生した場合:

```
A requires B ^1.0.0
C requires B ^2.0.0
```

エラーメッセージ:
```
error: failed to select a version for `B`
    required by package `A v1.0.0`
    required by package `C v1.0.0`
versions that meet the requirements `^1.0.0` are: 1.0.0, 1.1.0, 1.2.0
versions that meet the requirements `^2.0.0` are: 2.0.0, 2.1.0
```

---

## パッケージレジストリ

### アーキテクチャ

```
Registry Server
├── API Server (REST API)
│   ├── /api/v1/packages
│   ├── /api/v1/packages/{name}
│   ├── /api/v1/packages/{name}/versions
│   └── /api/v1/search
├── Storage
│   ├── Package Metadata (Database)
│   └── Package Archives (S3/Object Storage)
└── Web UI
    ├── Package Search
    ├── Package Details
    └── Documentation
```

### API エンドポイント

```
GET  /api/v1/packages                    # パッケージ一覧
GET  /api/v1/packages/{name}             # パッケージ詳細
GET  /api/v1/packages/{name}/versions    # バージョン一覧
GET  /api/v1/packages/{name}/{version}   # 特定バージョン詳細
POST /api/v1/packages                    # パッケージ公開
GET  /api/v1/search?q={query}           # パッケージ検索
```

### パッケージ形式

```
my-lib-1.0.0.cbpkg (tar.gz)
├── cb.toml
├── src/
│   └── lib.cb
├── README.md
└── LICENSE
```

---

## キャッシュシステム

### ローカルキャッシュ

```
~/.cbpkg/
├── registry/
│   └── index/                # レジストリインデックス
│       └── packages.json
├── cache/                    # ダウンロードキャッシュ
│   └── packages/
│       ├── http-client-1.0.0.cbpkg
│       └── json-parser-1.2.3.cbpkg
└── config.toml               # 設定ファイル
```

### キャッシュクリア

```bash
$ cbpkg cache clean
   Removing cached packages
     Cleaned 145 MB from cache
```

---

## ワークスペース

複数のパッケージを1つのリポジトリで管理：

### ディレクトリ構造

```
workspace/
├── cb.toml
├── packages/
│   ├── app/
│   │   ├── cb.toml
│   │   └── src/main.cb
│   ├── lib-core/
│   │   ├── cb.toml
│   │   └── src/lib.cb
│   └── lib-utils/
│       ├── cb.toml
│       └── src/lib.cb
```

### ルートcb.toml

```toml
[workspace]
members = [
    "packages/app",
    "packages/lib-core",
    "packages/lib-utils"
]

[workspace.dependencies]
# 共通の依存関係
log = "1.0.0"
```

### ワークスペースコマンド

```bash
$ cbpkg build --workspace       # 全てをビルド
$ cbpkg test --workspace        # 全てテスト
$ cbpkg publish --workspace     # 全て公開
```

---

## 実装スケジュール

### Month 1: コアシステム

**Week 1-2: パッケージマネージャー基礎**
- cbpkgコマンドラインツール
- cb.toml解析
- プロジェクト管理（new, init）

**Week 3: 依存関係解決**
- バージョン解析
- 依存関係グラフ構築
- 競合検出

**Week 4: ビルドシステム統合**
- cbpkg buildコマンド
- キャッシュシステム
- インクリメンタルビルド

### Month 2: レジストリとエコシステム

**Week 1-2: レジストリサーバー**
- REST API実装
- データベース設計
- ストレージシステム

**Week 3: クライアント実装**
- パッケージ公開機能
- 検索機能
- 認証システム

**Week 4: Web UI**
- パッケージ検索UI
- ドキュメントビューワー
- ユーザーアカウント管理

### Month 3: テストと最適化

**Week 1-2: テストとベンチマーク**
- 統合テスト
- パフォーマンステスト
- セキュリティ監査

**Week 3: ドキュメント**
- ユーザーガイド
- API ドキュメント
- チュートリアル

**Week 4: リリース準備**
- バグ修正
- 最適化
- ベータテスト

---

## 完了条件

v0.18.0は以下の条件を満たしたときに完了とします：

1. **機能完全性**
   - [ ] cbpkg コマンドが全て動作
   - [ ] 依存関係解決が正しく動作
   - [ ] パッケージレジストリが稼働
   - [ ] ビルドシステムが統合されている

2. **エコシステム**
   - [ ] 10個以上のパッケージが公開されている
   - [ ] ドキュメントが完備
   - [ ] サンプルプロジェクトが動作

3. **品質**
   - [ ] 全てのテストがパス
   - [ ] パフォーマンスが許容範囲
   - [ ] セキュリティ監査完了

---

## 次のバージョン（v0.19.0以降）

将来のバージョンで実装予定：

- プライベートレジストリ
- ミラーサーバー
- CI/CD統合
- IDE統合（Language Server Protocol）
- コード補完とリファクタリング
