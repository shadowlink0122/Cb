# v0.14.0 実装計画まとめ

**作成日**: 2025-11-16
**ステータス**: 実装中

---

## v0.14.0の目標

### IR（中間表現）基盤の構築

v0.14.0では、Cb言語のコンパイラ基盤としてIR（中間表現）を実装します。
これにより、インタプリタからコンパイラへの移行が可能になります。

**主要機能**:
- ✓ FFI (Foreign Function Interface) - C関数呼び出し
- ✓ 条件付きコンパイル (#[cfg(...)]) - プラットフォーム分岐
- ✓ モジュールシステム (mod/use) - コード整理とインポート

以下の追加設計により、全ての要件をサポートします：

---

## 1. OS開発・低レイヤアプリケーション ✓

### サポートする機能

#### ベアメタル実行
- ✓ OSなし実行環境
- ✓ カスタムメモリレイアウト（RAM/ROM開始アドレス、サイズ指定）
- ✓ カスタムエントリーポイント
- ✓ スタートアップコード

#### インラインアセンブラ
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

**対応アーキテクチャ**:
- x86-64 (AT&T/Intel構文)
- ARM/ARM64 (AAPCS)
- RISC-V

#### メモリマップドIO
```cb
void uart_send(char c) {
    UART* uart = 0x40000000 as UART*;
    volatile {
        while ((uart->status & 0x80) == 0) {}
        uart->data = c as int;
    }
}
```

#### 割り込みハンドラ
```cb
#[interrupt(irq = 16)]
void timer_handler() {
    // タイマー割り込み処理
}

#[exception]
void HardFault_Handler() {
    while (1) {}
}
```

#### リンカースクリプト生成
```bash
./main firmware.cb \
    --backend=native \
    --target=arm-none-eabi \
    --environment=freestanding \
    --emit-linker-script=firmware.ld
```

#### OS開発用組み込み関数
- ポート入出力（`outb`, `inb`）
- MSR読み書き（`rdmsr`, `wrmsr`）
- メモリバリア
- アトミック操作
- ページテーブル操作
- GDT/IDT設定

### コマンドライン例

```bash
# ARM Cortex-M用ファームウェアコンパイル
./main firmware.cb \
    --backend=native \
    --target=arm-none-eabi \
    --environment=freestanding \
    --ram-start=0x20000000 \
    --ram-size=128K \
    --rom-start=0x08000000 \
    --rom-size=512K \
    --output=firmware.elf

# RISC-V用カーネルコンパイル
./main kernel.cb \
    --backend=native \
    --target=riscv64-unknown-none \
    --environment=freestanding \
    --output=kernel.elf
```

---

## 2. Webフロントエンド開発 ✓

### サポートする機能

#### HTML生成
```cb
Html render_page() {
    return html {
        head {
            title { "My Cb App" }
            link(rel="stylesheet", href="style.css")
        }
        body {
            div(class="container") {
                h1 { "Hello, Cb!" }
                button(onclick=handle_click) { "Click me!" }
            }
        }
    };
}

// JSX/TSX風の構文
Html render_component() {
    return jsx! {
        <div class="card">
            <h2>{title}</h2>
            <button onclick={on_click}>Submit</button>
        </div>
    };
}
```

#### CSS生成
```cb
StyleSheet define_styles() {
    return css {
        ".container" {
            display: "flex";
            flex_direction: "column";
            padding: px(20);
            background_color: rgb(255, 255, 255);
        }

        ".btn" {
            background_color: rgb(0, 122, 255);
            padding: px(10) px(20);
            border_radius: px(4);

            "&:hover" {
                background_color: rgb(0, 100, 230);
            }
        }
    };
}
```

#### コンポーネントシステム
```cb
struct TodoApp {
    Vec<Todo> todos;

    Html render(self) {
        return div(class="app") {
            h1 { "Cb Todo App" }
            self.render_input()
            self.render_list()
        };
    }

    Html render_input(self) {
        return input(
            type="text",
            placeholder="Add todo...",
            onkeypress=|e| {
                if (e.key == "Enter") {
                    self.add_todo(e.target.value);
                }
            }
        );
    }

    Html render_list(self) {
        return ul {
            for (todo in self.todos) {
                li(key=todo.id) {
                    input(
                        type="checkbox",
                        checked=todo.completed,
                        onchange=|_| self.toggle(todo.id)
                    )
                    span { todo.text }
                }
            }
        };
    }
};
```

#### 状態管理
```cb
// Redux風の状態管理
enum Action {
    AddTodo(string),
    ToggleTodo(int),
    RemoveTodo(int),
};

TodoState todo_reducer(TodoState state, Action action) {
    match (action) {
        Action::AddTodo(text) => {
            // 新しいTodoを追加
        }
        Action::ToggleTodo(id) => {
            // Todoの完了状態を切り替え
        }
        // ...
    }
}

// リアクティブな状態
struct AppState {
    @reactive
    string name;

    @reactive
    int count;
};
```

### コマンドライン例

```bash
# WASM + HTML + CSS生成
./main app.cb \
    --backend=wasm \
    --target=wasm32 \
    --output=dist/app.wasm \
    --emit-html \
    --emit-css

# TypeScript + HTML + CSS生成
./main app.cb \
    --backend=typescript \
    --output=dist/app.ts \
    --emit-html \
    --emit-css

# 開発サーバー起動
./main app.cb --serve --watch --port=3000
```

---

## 3. 複数バックエンド対応 ✓

### サポートするバックエンド

| バックエンド | 出力形式 | 主な用途 | 追加機能 |
|------------|---------|----------|---------|
| **Interpreter** | - | デバッグ、即座実行 | - |
| **Native** | ELF/Mach-O/PE | 高性能アプリ、OS開発 | **インラインアセンブラ、ベアメタル** |
| **WASM** | .wasm | Webアプリ | **HTML/CSS生成** |
| **TypeScript** | .ts | フロントエンド開発 | **HTML/CSS生成、コンポーネント** |

### アーキテクチャ

```
Frontend (Parser)
    │
    ▼
IR Pipeline (AST → HIR → MIR → LIR)
    │
    ▼
Backend Dispatcher
    │
    ├─→ Interpreter ────→ Execute
    ├─→ Native ─────────→ Binary (x86-64/ARM64)
    ├─→ WASM ───────────→ .wasm + HTML + CSS
    └─→ TypeScript ─────→ .ts + HTML + CSS
```

---

## 4. ターゲット一覧

### ネイティブターゲット

| ターゲット | アーキテクチャ | OS | 用途 |
|-----------|--------------|-----|------|
| `x86_64-unknown-linux-gnu` | x86-64 | Linux | 通常アプリ |
| `x86_64-apple-darwin` | x86-64 | macOS | 通常アプリ |
| `x86_64-pc-windows-msvc` | x86-64 | Windows | 通常アプリ |
| `arm64-apple-darwin` | ARM64 | macOS | Apple Silicon |
| `arm64-unknown-linux-gnu` | ARM64 | Linux | ARM Linux |
| `arm-none-eabi` | ARM Cortex-M | ベアメタル | 組み込み/OS開発 |
| `riscv64-unknown-none` | RISC-V | ベアメタル | OS開発 |

### Webターゲット

| ターゲット | 出力 | 実行環境 |
|-----------|-----|---------|
| `wasm32-unknown-unknown` | .wasm | ブラウザ/Node.js |
| `wasm64-unknown-unknown` | .wasm | ブラウザ/Node.js |
| `typescript` | .ts | ブラウザ/Node.js |

---

## 5. プロジェクト例

### OS開発プロジェクト

```
my-os/
├── src/
│   ├── boot/
│   │   └── boot.cb          # ブートローダー
│   ├── kernel/
│   │   ├── main.cb          # カーネルエントリー
│   │   ├── memory.cb        # メモリ管理
│   │   ├── interrupt.cb     # 割り込み処理
│   │   └── scheduler.cb     # スケジューラ
│   └── drivers/
│       ├── uart.cb          # UARTドライバ
│       └── timer.cb         # タイマードライバ
├── linker.ld                # リンカースクリプト
└── cb.toml                  # プロジェクト設定
```

**ビルド**:
```bash
./main src/kernel/main.cb \
    --backend=native \
    --target=arm-none-eabi \
    --environment=freestanding \
    --linker-script=linker.ld \
    --output=kernel.elf
```

### Webアプリプロジェクト

```
my-web-app/
├── src/
│   ├── main.cb              # エントリーポイント
│   ├── components/
│   │   ├── Header.cb
│   │   ├── TodoList.cb
│   │   └── Footer.cb
│   ├── styles/
│   │   └── app.cb           # CSS定義
│   └── state/
│       └── store.cb         # 状態管理
├── public/
│   └── index.html           # ベースHTML
└── cb.toml                  # プロジェクト設定
```

**ビルド**:
```bash
./main src/main.cb \
    --backend=wasm \
    --output=dist/app.wasm \
    --emit-html \
    --emit-css
```

---

## 6. 実装スケジュール

### Month 1: IR実装
- Week 1-2: HIR実装
- Week 3: HIR高度な機能
- Week 4: HIRテスト

### Month 2: MIR実装
- Week 5-6: MIR基本構造とCFG
- Week 7: SSA形式
- Week 8: データフロー解析とテスト

### Month 3: LIR + FFI + モジュールシステム
- Week 9-10: LIR実装
- **Week 11: FFIと条件付きコンパイル（次バージョン（v0.15.0以降）への準備）**
- **Week 12: モジュールシステムと統合**

### Month 4: バックエンド実装
- **Week 13-14: ネイティブバックエンド + インラインアセンブラ + ベアメタル対応**
- **Week 15-16: WASM/TypeScriptバックエンド + HTML/CSS生成**

---

## 7. 作成したドキュメント

### v0.14.0 基本設計（8ドキュメント）

1. **README.md** - v0.14.0の概要
2. **ir_implementation_plan.md** - IR技術選定と設計
3. **implementation_roadmap.md** - 実装タスクとスケジュール（更新：FFI/モジュール追加）
4. **detailed_design.md** - 各タスクの詳細設計
5. **refactoring_plan.md** - リファクタリング計画
6. **multi_backend_architecture.md** - 複数バックエンド対応
7. **wasm_backend_design.md** - WASM対応
8. **typescript_backend_design.md** - TypeScript対応

### v0.14.0 追加設計（3ドキュメント）

9. **low_level_support.md** - 低レイヤアプリケーション対応
   - ベアメタル実行
   - インラインアセンブラ
   - メモリマップドIO
   - 割り込みハンドラ
   - OS開発用機能

10. **web_frontend_support.md** - Webフロントエンド開発
    - HTML生成機能
    - CSS生成機能
    - コンポーネントシステム
    - 状態管理

11. **SUMMARY.md** - v0.14.0総合まとめ（このドキュメント）

### v0.17.0 設計（2ドキュメント）

12. **v0.17.0/README.md** - v0.17.0概要（標準ライブラリ化）
13. **v0.17.0/stdlib_design.md** - 標準ライブラリ詳細設計
    - FFI実装
    - 条件付きコンパイル
    - モジュールシステム
    - プラットフォーム固有実装

### v0.18.0 設計（1ドキュメント）

14. **v0.18.0/README.md** - v0.18.0概要（パッケージ管理）
    - パッケージマネージャー（cbpkg）
    - 依存関係解決
    - パッケージレジストリ

### 総合ロードマップ（1ドキュメント）

15. **ROADMAP_v0.14-v0.18_SUMMARY.md** - 3バージョン統合ロードマップ

---

## 8. まとめ

### v0.14.0で実現すること

**✓ OS開発・組み込みシステム**
- ベアメタル実行環境
- インラインアセンブラ（x86-64/ARM/RISC-V）
- メモリマップドIO
- 割り込みハンドラ
- カスタムリンカースクリプト

**✓ 高性能ネイティブアプリケーション**
- x86-64/ARM64ネイティブコード生成
- Linux/macOS/Windowsサポート
- 最適化パイプライン

**✓ Webフロントエンド開発**
- HTML生成（テンプレート/JSX風）
- CSS生成（型安全なスタイリング）
- コンポーネントベース開発
- リアクティブな状態管理
- WASM/TypeScript出力

**✓ クロスプラットフォーム開発**
- 単一のCbコードから複数ターゲットへ出力
- 統一されたAPIと開発体験
- ターゲット別の最適化

**✓ 次バージョン（v0.15.0以降）への準備機能**
- FFI (Foreign Function Interface) - C関数呼び出し
- 条件付きコンパイル (#[cfg(...)]) - プラットフォーム分岐
- モジュールシステム (mod/use) - コード整理とインポート

### 実装期間

**合計**: 3-4ヶ月

**完了後の状態**:
- Cb言語で**OS開発**が可能
- Cb言語で**Webアプリ開発**が可能
- Cb言語で**組み込みシステム開発**が可能
- Cb言語で**高性能ネイティブアプリ開発**が可能
- **将来のバージョン（v0.15.0: 複数バックエンド、v0.16.0、v0.17.0: 標準ライブラリ化）への準備が完了**

### 次のバージョン

**v0.15.0（3-4ヶ月）**: 複数バックエンド対応（ネイティブ、WASM、TypeScript）
**v0.17.0（2-3ヶ月）**: 標準ライブラリのライブラリ化
- OS依存機能の標準ライブラリ化（println, malloc等）
- プラットフォーム固有実装（Linux/macOS/Windows/ベアメタル）
- FFI、条件付きコンパイル、インラインアセンブラの実戦使用

**v0.18.0（2-3ヶ月）**: パッケージエコシステム
- パッケージマネージャー（cbpkg）
- 依存関係管理とバージョン解決
- パッケージレジストリ

### 総合タイムライン

**8-10ヶ月で実現**:
- 完全なネイティブコンパイラ
- ポータブルな標準ライブラリ
- パッケージエコシステム
- 開発者コミュニティの形成
