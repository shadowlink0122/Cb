# Cb言語開発ロードマップ v0.16.0 - v0.18.0 総合まとめ

**作成日**: 2025-11-14
**対象期間**: 約8-10ヶ月

---

## エグゼクティブサマリー

このドキュメントは、Cb言語の次期3つのメジャーバージョン（v0.16.0、v0.17.0、v0.18.0）の実装計画をまとめたものです。これらのバージョンで、Cb言語は以下の目標を達成します：

1. **v0.16.0**: ネイティブコンパイラの基盤構築
2. **v0.17.0**: 標準ライブラリのライブラリ化
3. **v0.18.0**: パッケージエコシステムの確立

---

## 全体タイムライン

```
Month 1-4: v0.16.0 開発
    ├─ Month 1: HIR実装
    ├─ Month 2: MIR実装
    ├─ Month 3: LIR + FFI + モジュールシステム
    └─ Month 4: バックエンド実装と統合

Month 5-7: v0.17.0 開発
    ├─ Month 5: 標準ライブラリ設計と基盤実装
    ├─ Month 6: プラットフォーム固有実装
    └─ Month 7: テストとドキュメント

Month 8-10: v0.18.0 開発
    ├─ Month 8: パッケージマネージャー実装
    ├─ Month 9: レジストリとエコシステム
    └─ Month 10: テストと最適化
```

---

## v0.16.0: ネイティブコンパイラ基盤（3-4ヶ月）

### 概要

v0.16.0は、Cb言語の本格的なネイティブコンパイラ実装の基盤となるバージョンです。インタプリタ中心のアーキテクチャから、コンパイラベースのアーキテクチャへの大規模な移行を開始します。

### 主要な実装内容

#### 1. 3層IR構造（HIR/MIR/LIR）

**HIR (High-level IR)**
- ASTに型情報とスコープ情報を付加
- ジェネリクスの単相化（Monomorphization）
- 型推論の実行

**MIR (Mid-level IR)**
- SSA形式の実装
- 制御フローグラフ（CFG）の構築
- データフロー解析（生存変数解析、到達定義解析）
- 支配木の構築（Lengauer-Tarjanアルゴリズム）

**LIR (Low-level IR)**
- ターゲット非依存の3アドレスコード
- 仮想レジスタの管理
- バックエンドへの橋渡し

#### 2. 複数バックエンド対応

**ネイティブバックエンド**
- x86-64（Linux/macOS/Windows）
- ARM64（Apple Silicon、ARM Linux）
- ARM Cortex-M（組み込み/OS開発）
- RISC-V（OS開発）

**Webバックエンド**
- WASM（wasm32/wasm64）
- TypeScript変換

#### 3. 低レイヤアプリケーション対応

**インラインアセンブラ**
```cb
ulong read_cr0() {
    long result;
    asm volatile (
        "mov %cr0, %rax"
        : "=r"(result)
        :
        : "rax"
    );
    return result;
}
```

**ベアメタル実行**
- カスタムメモリレイアウト
- カスタムエントリーポイント
- リンカースクリプト生成

**メモリマップドIO**
```cb
void uart_send(char c) {
    UART* uart = 0x40000000 as UART*;
    volatile {
        uart->data = c as int;
    }
}
```

**割り込みハンドラ**
```cb
#[interrupt(irq = 16)]
void timer_handler() {
    // タイマー割り込み処理
}
```

#### 4. Webフロントエンド開発

**HTML生成**
```cb
Html render() {
    return div(class="app") {
        h1 { "Hello, Cb!" }
        button(onclick=handler) { "Click me!" }
    };
}
```

**CSS生成**
```cb
StyleSheet styles() {
    return css {
        ".app" {
            padding: px(20);
            background_color: rgb(255, 255, 255);
        }
    };
}
```

**コンポーネントシステム**
- Reactスタイルのコンポーネント
- リアクティブな状態管理
- イベントハンドリング

#### 5. v0.17.0準備機能

**FFI (Foreign Function Interface)**
```cb
extern "C" {
    void* malloc(long size);
    void free(void* ptr);
}
```

**条件付きコンパイル**
```cb
#[cfg(target_os = "linux")]
void platform_specific() {
    // Linux固有の実装
}

#[cfg(target_os = "windows")]
void platform_specific() {
    // Windows固有の実装
}
```

**モジュールシステム**
```cb
mod math {
    export int add(int a, int b) {
        return a + b;
    }
}

use math::add;
```

### 実装スケジュール

#### Month 1: HIR実装
- Week 1-2: HIR基本構造
- Week 3: HIR高度な機能
- Week 4: HIRジェネリクスとテスト

#### Month 2: MIR実装
- Week 1: MIR基本構造とCFG
- Week 2: SSA形式の実装
- Week 3: データフロー解析
- Week 4: MIR完成とテスト

#### Month 3: LIR + FFI + モジュールシステム
- Week 1-2: LIR実装
- Week 3: FFIと条件付きコンパイル
- Week 4: モジュールシステムと統合

#### Month 4: バックエンド実装（追加）
- Week 1-2: ネイティブバックエンド + インラインアセンブラ + ベアメタル対応
- Week 3-4: WASM/TypeScriptバックエンド + HTML/CSS生成

### 成果物

- 44ファイルのC++実装
- 155以上のユニットテスト
- 20以上の統合テスト
- 9つの仕様書/設計ドキュメント
- IRビューワー、CFG可視化ツール

### 完了条件

- [x] HIR/MIR/LIRの完全な実装
- [x] 複数バックエンド対応
- [x] FFI、条件付きコンパイル、モジュールシステム
- [x] インラインアセンブラ、ベアメタル対応
- [x] HTML/CSS生成、コンポーネントシステム
- [x] 全テストパス（175テスト以上）
- [x] コードカバレッジ > 85%

---

## v0.17.0: 標準ライブラリのライブラリ化（2-3ヶ月）

### 概要

v0.17.0では、現在コンパイラに組み込まれているOS依存機能（println、mallocなど）を標準ライブラリとして分離・パッケージ化します。

### 主要な実装内容

#### 1. 標準ライブラリ構造

```
std/
├── io/           # 入出力機能
│   ├── stdio.cb  # 標準入出力
│   └── file.cb   # ファイルIO
├── mem/          # メモリ管理
│   ├── alloc.cb  # アロケーター
│   └── ptr.cb    # ポインタ操作
├── sys/          # システムコール
│   ├── linux.cb
│   ├── macos.cb
│   ├── windows.cb
│   └── none.cb   # ベアメタル
└── runtime/      # ランタイム
    ├── panic.cb
    └── start.cb
```

#### 2. OS抽象化レイヤ

**標準入出力（std::io::stdio.cb）**
```cb
export void println(string s) {
    sys::write(1, s, strlen(s));
    sys::write(1, "\n", 1);
}
```

**メモリ管理（std::mem::alloc.cb）**
```cb
export void* alloc(long size) {
    #[cfg(target_os = "linux")]
    return sys::linux::malloc(size);

    #[cfg(target_os = "windows")]
    return sys::windows::HeapAlloc(size);

    #[cfg(target_os = "none")]
    return sys::none::simple_alloc(size);
}
```

#### 3. プラットフォーム固有実装

**Linux実装（インラインアセンブラ + システムコール）**
```cb
export long write(int fd, char* buf, long count) {
    long result;
    asm volatile (
        "mov $1, %rax\n"       // SYS_WRITE
        "mov %0, %rdi\n"       // fd
        "mov %1, %rsi\n"       // buf
        "mov %2, %rdx\n"       // count
        "syscall"
        : "=r"(result)
        : "r"(fd), "r"(buf), "r"(count)
        : "rax", "rdi", "rsi", "rdx", "r11", "rcx", "memory"
    );
    return result;
}
```

**Windows実装（FFI + Windows API）**
```cb
extern "C" {
    #[link_name = "WriteFile"]
    export int WriteFile(
        void* hFile,
        void* lpBuffer,
        int nNumberOfBytesToWrite,
        int* lpNumberOfBytesWritten,
        void* lpOverlapped
    );
}
```

**ベアメタル実装（バンプアロケーター）**
```cb
static long HEAP_CURRENT = 0;

export void* simple_alloc(long size) {
    void* ptr = HEAP_CURRENT as void*;
    HEAP_CURRENT += size;
    return ptr;
}
```

### 実装スケジュール

#### Month 1: 標準ライブラリ基盤
- Week 1-2: 入出力ライブラリ（stdio）
- Week 3: メモリ管理ライブラリ（alloc）
- Week 4: ランタイムサポート

#### Month 2: プラットフォーム対応
- Week 1: Linux実装
- Week 2: macOS/Windows実装
- Week 3: ベアメタル実装
- Week 4: システムコールラッパー

#### Month 3: テストとドキュメント
- Week 1-2: 各プラットフォームでのテスト
- Week 3: ドキュメント作成
- Week 4: サンプルプロジェクト作成

### 成果物

- 完全なプラットフォーム独立標準ライブラリ
- Linux/macOS/Windows/ベアメタル対応
- 100以上のライブラリテスト
- APIリファレンス

### 完了条件

- [x] 標準ライブラリが全プラットフォームで動作
- [x] FFI、条件付きコンパイル、インラインアセンブラの実戦使用
- [x] ベアメタル環境でも動作
- [x] 既存コードとの後方互換性

---

## v0.18.0: パッケージエコシステム（2-3ヶ月）

### 概要

v0.18.0では、Cb言語のエコシステムを確立し、パッケージ管理システムを実装します。

### 主要な実装内容

#### 1. パッケージマネージャー（cbpkg）

**基本コマンド**
```bash
# プロジェクト管理
cbpkg new my-project
cbpkg init

# 依存関係管理
cbpkg add http-client
cbpkg add json-parser@1.2.3

# ビルドと実行
cbpkg build
cbpkg run
cbpkg test

# パッケージ公開
cbpkg publish
```

#### 2. プロジェクト設定（cb.toml）

```toml
[package]
name = "my-project"
version = "0.1.0"
authors = ["Your Name <you@example.com>"]
edition = "2025"

[dependencies]
http-client = "1.0.0"
json-parser = "^1.2.0"

[dev-dependencies]
test-framework = "0.5.0"

[build]
target = "x86_64-unknown-linux-gnu"
backend = "native"
```

#### 3. 依存関係解決

**セマンティックバージョニング**
```toml
# バージョン範囲指定
caret = "^1.2.3"    # 1.2.3 <= version < 2.0.0
tilde = "~1.2.3"    # 1.2.3 <= version < 1.3.0
exact = "1.2.3"     # 完全一致
```

**依存関係グラフ**
```
my-project (0.1.0)
├── http-client (1.0.0)
│   ├── tcp-socket (2.3.1)
│   └── tls (1.5.0)
└── json-parser (1.2.3)
```

#### 4. パッケージレジストリ

**レジストリサーバー**
- REST API
- パッケージ検索
- バージョン管理
- ダウンロード統計
- Web UI

**API エンドポイント**
```
GET  /api/v1/packages
GET  /api/v1/packages/{name}
POST /api/v1/packages
GET  /api/v1/search?q={query}
```

#### 5. ビルドシステム統合

```bash
# 複数バックエンド対応
cbpkg build --backend=native
cbpkg build --backend=wasm
cbpkg build --backend=typescript

# クロスコンパイル
cbpkg build --target=arm-none-eabi
cbpkg build --target=wasm32-unknown-unknown
```

#### 6. ワークスペース

```toml
[workspace]
members = [
    "packages/app",
    "packages/lib-core",
    "packages/lib-utils"
]
```

### 実装スケジュール

#### Month 1: コアシステム
- Week 1-2: パッケージマネージャー基礎
- Week 3: 依存関係解決
- Week 4: ビルドシステム統合

#### Month 2: レジストリとエコシステム
- Week 1-2: レジストリサーバー
- Week 3: クライアント実装
- Week 4: Web UI

#### Month 3: テストと最適化
- Week 1-2: テストとベンチマーク
- Week 3: ドキュメント
- Week 4: リリース準備

### 成果物

- cbpkg パッケージマネージャー
- パッケージレジストリ（サーバー + クライアント）
- Web UI
- 10個以上の公開パッケージ

### 完了条件

- [x] cbpkgコマンドが全て動作
- [x] 依存関係解決が正しく動作
- [x] パッケージレジストリが稼働
- [x] エコシステムが機能（10個以上のパッケージ）

---

## 統合されたビジョン

### v0.16.0完了時点

✓ ネイティブコンパイラ基盤
✓ 複数バックエンド（Native/WASM/TypeScript）
✓ OS開発・組み込みシステム対応
✓ Webフロントエンド開発対応
✓ FFI、条件付きコンパイル、モジュールシステム

**Cb言語でできること**:
- OS開発（ベアメタル、インラインアセンブラ）
- 高性能ネイティブアプリ（Linux/macOS/Windows）
- Webアプリ開発（WASM、TypeScript、HTML/CSS生成）
- 組み込みシステム開発（ARM Cortex-M、RISC-V）

### v0.17.0完了時点

✓ 標準ライブラリのライブラリ化
✓ プラットフォーム独立実装
✓ OS抽象化レイヤ

**Cb言語でできること**:
- v0.16.0の全て
- + ポータブルな標準ライブラリ
- + プラットフォーム固有の最適化実装
- + カスタムアロケーター

### v0.18.0完了時点

✓ パッケージエコシステム
✓ パッケージマネージャー
✓ 依存関係管理

**Cb言語でできること**:
- v0.17.0の全て
- + ライブラリの簡単な共有・利用
- + 依存関係の自動解決
- + プロジェクト管理の簡素化

---

## ユースケース例

### 1. OS開発プロジェクト（v0.16.0〜）

```bash
# プロジェクト作成
cbpkg new my-os --no-std

# コンパイル
./main src/kernel/main.cb \
    --backend=native \
    --target=arm-none-eabi \
    --environment=freestanding \
    --output=kernel.elf
```

**main.cb**:
```cb
#[no_mangle]
fn Reset_Handler() -> ! {
    // スタートアップコード
    init_memory();
    init_interrupts();
    main();
    loop {}
}

fn main() {
    uart_println("Hello from Cb OS!");
}
```

### 2. Webアプリ開発（v0.16.0〜）

```bash
# プロジェクト作成
cbpkg new my-web-app --backend=wasm

# 開発サーバー起動
cbpkg run --serve --watch
```

**app.cb**:
```cb
fn render() -> Html {
    return div(class="app") {
        h1 { "Todo App" }
        TodoList { todos: app_state.todos }
    };
}
```

### 3. クロスプラットフォームライブラリ（v0.17.0〜）

```bash
# ライブラリプロジェクト作成
cbpkg new my-lib --lib

# 公開
cbpkg publish
```

**lib.cb**:
```cb
#[cfg(target_os = "linux")]
pub fn platform_name() -> string {
    return "Linux";
}

#[cfg(target_os = "windows")]
pub fn platform_name() -> string {
    return "Windows";
}

#[cfg(target_os = "none")]
pub fn platform_name() -> string {
    return "Baremetal";
}
```

### 4. 依存関係のあるアプリケーション（v0.18.0〜）

```bash
# プロジェクト作成
cbpkg new my-app

# 依存関係追加
cbpkg add http-client
cbpkg add json-parser

# ビルドと実行
cbpkg build --release
cbpkg run
```

**cb.toml**:
```toml
[package]
name = "my-app"
version = "0.1.0"

[dependencies]
http-client = "^1.0.0"
json-parser = "^1.2.0"
```

**main.cb**:
```cb
use http::Client;
use json::parse;

fn main() {
    let client = Client::new();
    let response = client.get("https://api.example.com/data");
    let data = parse(response.body);
    println("Data: {}", data);
}
```

---

## リスク管理

### 技術的リスク

| リスク | 影響 | 対策 |
|--------|------|------|
| SSA変換の複雑性 | 高 | Lengauer-Tarjanアルゴリズムの実績ある実装を参考 |
| 複数バックエンドの保守コスト | 中 | 共通インターフェース設計、テスト自動化 |
| FFIのセキュリティ | 高 | 型チェックの厳格化、サンドボックス化 |
| パッケージレジストリのスケーラビリティ | 中 | CDN活用、キャッシュ戦略 |

### スケジュールリスク

| リスク | 確率 | 対策 |
|--------|------|------|
| IR実装の遅延 | 中 | 段階的実装、早期プロトタイピング |
| バックエンド実装の遅延 | 低 | 既存ツール（LLVM等）の活用も検討 |
| 標準ライブラリのテスト不足 | 中 | CI/CD自動化、プラットフォーム別テスト |

---

## 成功指標（KPI）

### v0.16.0
- [ ] 全テスト155個以上でパス率100%
- [ ] コードカバレッジ > 85%
- [ ] 1000行のコード処理時間 < 200ms
- [ ] メモリリークゼロ

### v0.17.0
- [ ] 標準ライブラリが4プラットフォームで動作
- [ ] 全ライブラリテストパス（100テスト以上）
- [ ] 後方互換性100%

### v0.18.0
- [ ] cbpkgコマンド全機能動作
- [ ] パッケージレジストリ稼働
- [ ] 10個以上のパッケージ公開
- [ ] 100ユーザー以上が利用

---

## 結論

v0.16.0からv0.18.0までの実装により、Cb言語は以下を達成します：

1. **汎用性**: OS開発からWebアプリ開発まで対応
2. **移植性**: 複数プラットフォームで動作する標準ライブラリ
3. **エコシステム**: パッケージ管理による開発者コミュニティの形成

**総実装期間**: 8-10ヶ月
**総テスト数**: 400テスト以上
**総ドキュメント**: 20ドキュメント以上

この計画により、Cb言語は実用的なプログラミング言語として完成します。
