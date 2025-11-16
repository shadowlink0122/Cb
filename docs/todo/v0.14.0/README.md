# v0.14.0 - IR（中間表現）実装と複数バックエンド対応

**バージョン**: v0.14.0
**目標**: コンパイラの中間表現（IR）を設計・実装し、複数のターゲットをサポート
**期間**: 3-4ヶ月
**ステータス**: 計画中

---

## 概要

v0.14.0は、Cb言語のネイティブコンパイラ実装における最初の大きなマイルストーンです。このバージョンでは、コンパイラの中間表現（IR）を3層構造（HIR/MIR/LIR）で設計・実装し、**複数のバックエンド（Native/WASM/TypeScript）と低レイヤアプリケーション開発、Webフロントエンド開発をサポート**します。

**このバージョンは大規模リファクタリングを含むため、マイナーバージョンとして設定されています。**

### サポートするユースケース

v0.14.0は以下の全てのユースケースをサポートします：

1. **OS開発・組み込みシステム**: ベアメタル実行、インラインアセンブラ、メモリマップドIO
2. **高性能アプリケーション**: ネイティブバイナリ（Linux/macOS/Windows）
3. **Webフロントエンド開発**: HTML/CSS生成、コンポーネントシステム、状態管理
4. **クロスプラットフォーム**: WASM対応、TypeScript変換

---

## 主要な成果物

### 1. IR（中間表現）層

#### HIR (High-level IR) - 高レベル中間表現
- ASTに型情報とスコープ情報を付加
- 制御フロー構造を保持（if/while/for等）
- ジェネリクスの単相化（Monomorphization）

#### MIR (Mid-level IR) - 中レベル中間表現
- SSA形式（Static Single Assignment）
- 制御フローグラフ（CFG）
- 基本ブロック（Basic Blocks）
- データフロー解析

#### LIR (Low-level IR) - 低レベル中間表現
- 3アドレスコード形式
- ターゲット非依存の低レベル命令
- 仮想レジスタの管理
- アセンブリ生成への準備

### 2. 複数バックエンド対応

#### Interpreter（既存）
- AST直接実行
- 即座実行、デバッグ用

#### Native Backend（新規）
- x86-64 / ARM64 ネイティブコード生成
- ELF/Mach-O/PE形式サポート
- ベアメタル実行サポート
- **インラインアセンブラ対応**
- **メモリマップドIO対応**

#### WASM Backend（新規）
- WebAssembly バイナリ生成
- ブラウザ/Node.js実行
- JavaScript/TypeScript統合

#### TypeScript Backend（新規）
- TypeScript コード生成
- **HTML/CSS生成機能**
- **コンポーネントシステム**
- **リアクティブな状態管理**

### 3. 低レイヤアプリケーション開発機能

- ✓ ベアメタル実行環境
- ✓ インラインアセンブラ（`asm` 構文）
- ✓ Volatile メモリアクセス
- ✓ メモリマップドIO
- ✓ 割り込みハンドラ属性
- ✓ カスタムリンカースクリプト生成
- ✓ OS開発用組み込み関数

### 4. Webフロントエンド開発機能

- ✓ HTML生成（テンプレート構文）
- ✓ CSS生成（型安全なスタイリング）
- ✓ JSX/TSX風のコンポーネント構文
- ✓ DOMバインディング
- ✓ イベントハンドリング
- ✓ Redux風の状態管理

### 5. データフロー解析基盤
- 生存変数解析（Liveness Analysis）
- 到達定義解析（Reaching Definitions）
- 使用-定義チェーン（Use-Def Chain）
- 支配木（Dominator Tree）

### 6. IRビューワーとデバッグツール
- HIR/MIR/LIRのテキストダンプ
- GraphVizによるCFG可視化
- 支配木の可視化
- IR検証ツール

---

## ドキュメント

### 技術選定と設計
- **[ir_implementation_plan.md](./ir_implementation_plan.md)**: 詳細な技術選定とIR設計
  - IR設計アプローチ
  - SSA形式の実装方法
  - データ構造とメモリ管理
  - CFGの表現
  - HIR/MIR/LIRの詳細設計
  - テストフレームワーク

### 実装計画
- **[implementation_roadmap.md](./implementation_roadmap.md)**: 実装タスクとスケジュール
  - やらなければならないことの詳細リスト
  - 実装フェーズと週次スケジュール
  - チェックリスト
  - 完了条件

- **[detailed_design.md](./detailed_design.md)**: 各タスクの詳細設計
  - HIR/MIR/LIRノード構造の完全な定義
  - C++実装コード付き
  - データフロー解析の詳細

- **[refactoring_plan.md](./refactoring_plan.md)**: リファクタリング計画
  - 変更が必要な全ファイルのリスト
  - 既存コードへの影響範囲
  - 後方互換性の保証

### バックエンド設計
- **[multi_backend_architecture.md](./multi_backend_architecture.md)**: 複数バックエンド対応
  - 統一バックエンドインターフェース
  - バックエンドファクトリパターン
  - ターゲット情報管理

- **[wasm_backend_design.md](./wasm_backend_design.md)**: WASM対応
  - WASMバイナリフォーマット
  - LIRからWASM命令へのマッピング
  - JavaScript/TypeScript統合

- **[typescript_backend_design.md](./typescript_backend_design.md)**: TypeScript対応
  - LIRからTypeScriptへの変換
  - ポインタのエミュレーション
  - ランタイムライブラリ

### 低レイヤ・Webフロントエンド対応
- **[low_level_support.md](./low_level_support.md)**: 低レイヤアプリケーション対応
  - ベアメタル実行サポート
  - インラインアセンブラ
  - メモリマップドIO
  - 割り込みハンドラ
  - OS開発用機能

- **[web_frontend_support.md](./web_frontend_support.md)**: Webフロントエンド開発
  - HTML生成機能
  - CSS生成機能
  - コンポーネントシステム
  - 状態管理

---

## 実装スケジュール

### Month 1: HIR実装
**Week 1-2**: HIR基本構造
- HIRノード定義
- ASTからHIRへの変換
- 型情報の統合

**Week 3**: HIR高度な機能
- 制御フロー変換
- 関数・構造体の変換

**Week 4**: HIRジェネリクスとテスト
- ジェネリクス単相化
- HIRダンプ機能
- ユニットテスト（30テスト）

### Month 2: MIR実装
**Week 1**: MIR基本構造とCFG
- MIRノード定義
- CFG構築

**Week 2**: SSA形式
- 支配木構築
- PHIノード挿入
- 変数リネーミング

**Week 3**: データフロー解析
- 生存変数解析
- 到達定義解析
- 使用-定義チェーン

**Week 4**: MIR完成とテスト
- GraphViz可視化
- ユニットテスト（40テスト）

### Month 3: LIR実装とツール
**Week 1-2**: LIR実装
- LIRノード定義
- MIRからLIRへの変換
- ユニットテスト（30テスト）

**Week 3**: IRビューワーとツール
- ダンプ機能
- 可視化ツール
- IR検証ツール

**Week 4**: 統合とドキュメント
- 統合テスト（20テスト）
- ドキュメント完成
- リリース準備

---

## 技術スタック

### 実装言語とツール
- **言語**: C++17/20
- **ビルドシステム**: Make
- **テストフレームワーク**: Google Test
- **可視化**: GraphViz
- **メモリ管理**: std::unique_ptr, std::shared_ptr, アリーナアロケータ
- **多態性**: std::variant（仮想関数の代替）

### アルゴリズム
- **SSA構築**: Cytron et al.のアルゴリズム
- **支配木**: Lengauer-Tarjanアルゴリズム
- **データフロー解析**: 反復データフロー解析

---

## プロジェクト構造

```
src/backend/ir/
├── hir/
│   ├── hir_node.h/cpp         # HIRノード定義
│   ├── hir_generator.h/cpp    # ASTからHIRへの変換
│   ├── hir_visitor.h          # HIRビジター
│   └── hir_dumper.h/cpp       # HIRダンプ機能
├── mir/
│   ├── mir_node.h/cpp         # MIRノード定義
│   ├── mir_generator.h/cpp    # HIRからMIRへの変換
│   ├── cfg.h/cpp              # 制御フローグラフ
│   ├── ssa_builder.h/cpp      # SSA形式構築
│   ├── dominator_tree.h/cpp   # 支配木
│   ├── mir_dumper.h/cpp       # MIRダンプ機能
│   └── graphviz_gen.h/cpp     # GraphViz可視化
├── lir/
│   ├── lir_node.h/cpp         # LIRノード定義
│   ├── lir_generator.h/cpp    # MIRからLIRへの変換
│   └── lir_dumper.h/cpp       # LIRダンプ機能
└── analysis/
    ├── liveness.h/cpp         # 生存変数解析
    ├── reaching_defs.h/cpp    # 到達定義解析
    └── use_def_chain.h/cpp    # 使用-定義チェーン

tests/
├── unit/ir/                   # ユニットテスト
│   ├── test_hir_generation.cpp
│   ├── test_mir_generation.cpp
│   ├── test_lir_generation.cpp
│   ├── test_cfg_construction.cpp
│   ├── test_ssa_construction.cpp
│   └── test_dataflow_analysis.cpp
├── integration/ir/            # 統合テスト
│   ├── test_ir_roundtrip.cpp
│   └── test_ir_dump.cpp
└── cases/ir/                  # テストケース
    ├── simple_function.cb
    ├── control_flow.cb
    ├── nested_loops.cb
    └── generics.cb
```

---

## コマンドラインインターフェース

### 基本的な使用方法

```bash
# インタプリタで実行（デフォルト）
./main example.cb

# ネイティブバイナリにコンパイル
./main example.cb --backend=native --output=example

# WASMにコンパイル
./main example.cb --backend=wasm --output=example.wasm

# TypeScriptに変換
./main example.cb --backend=typescript --output=example.ts
```

### 低レイヤアプリケーション開発

```bash
# ベアメタル用ARM Cortex-Mコンパイル
./main firmware.cb \
    --backend=native \
    --target=arm-none-eabi \
    --environment=freestanding \
    --ram-start=0x20000000 \
    --ram-size=128K \
    --output=firmware.elf

# リンカースクリプト生成
./main firmware.cb \
    --backend=native \
    --emit-linker-script=firmware.ld
```

### Webフロントエンド開発

```bash
# Webアプリ用にビルド（WASM + HTML + CSS）
./main app.cb \
    --backend=wasm \
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
./main app.cb --serve --watch
```

### デバッグオプション

```bash
# HIRダンプ
./main example.cb --dump-hir

# MIRダンプ（CFG付き）
./main example.cb --dump-mir --dump-cfg

# LIRダンプ
./main example.cb --dump-lir

# GraphViz可視化
./main example.cb --emit-cfg-dot > output.dot
dot -Tpng output.dot -o output.png

# 全てのIRレベルをダンプ
./main example.cb --dump-all-ir

# IRレベルまで実行して停止
./main example.cb --stop-at=hir
./main example.cb --stop-at=mir
./main example.cb --stop-at=lir
```

---

## コード例

### 例1: シンプルな関数

#### ソースコード
```cb
int  add(x: int, y: int) {
    return x + y;
}
```

#### HIRダンプ
```
int  add(x: int, y: int) {
    return BinaryOp(+, Var(x: int), Var(y: int));
}
```

#### MIRダンプ
```
int  add(_0: int, _1: int) {
    let mut _2: int;  // return value
    let mut _3: int;  // temp

    bb0: {
        _3 = Add(_0, _1);
        _2 = _3;
        return _2;
    }
}
```

#### LIRダンプ
```
add:
    ; %0 = x (parameter)
    ; %1 = y (parameter)
    %2 = Add %0, %1
    Return %2
```

### 例2: インラインアセンブラ（OS開発）

```cb
// x86-64: CR0レジスタ読み取り
ulong  read_cr0() {
    long result;
    asm volatile (
        "mov %cr0, %rax"
        : "=r"(result)
        :
        : "rax"
    );
    return result;
}

// ARM: 割り込み有効化
#[interrupt]
fn timer_handler() {
    asm volatile ("cpsie i");
    // 割り込み処理...
}
```

### 例3: Webフロントエンド（Todoアプリ）

```cb
// コンポーネント定義
struct TodoApp {
    todos: Vec<Todo>;

    Html  render(self) {
        return div(class="app") {
            h1 { "Cb Todo App" }
            input(
                type="text",
                placeholder="Add todo...",
                onkeypress=self.handle_input
            )
            ul {
                for todo in self.todos {
                    li(key=todo.id) {
                        input(
                            type="checkbox",
                            checked=todo.completed,
                            onchange=|_| self.toggle(todo.id)
                        )
                        span { todo.text }
                    }
                }
            }
        };
    }
}

// スタイル定義
StyleSheet  styles() {
    return css {
        ".app" {
            max_width: px(600);
            margin: "0 auto";
            padding: px(20);
        }
        ".app h1" {
            color: rgb(0, 122, 255);
        }
    };
}
```

---

## テスト

### ユニットテスト
- **HIR**: 30テスト
- **MIR**: 40テスト
- **LIR**: 30テスト
- **合計**: 100+テスト

### 統合テスト
- **IRラウンドトリップ**: 20テスト
- **IRダンプ検証**: 含まれる

### ベンチマーク
- HIR生成のベンチマーク
- MIR生成のベンチマーク
- LIR生成のベンチマーク
- メモリ使用量のベンチマーク

---

## パフォーマンス目標

- **HIR生成**: 1000行のコードを100ms以内で処理
- **MIR生成**: HIRから50ms以内で変換
- **LIR生成**: MIRから30ms以内で変換
- **メモリ使用量**: 1000行のコードで50MB以内

---

## 完了条件

v0.14.0は以下の条件を満たしたときに完了：

1. **機能完全性**
   - HIR/MIR/LIRの全実装完了
   - ASTからLIRまでの完全な変換パイプライン
   - SSA形式の正しい実装

2. **品質保証**
   - 全てのユニットテストがパス（100+テスト）
   - コードカバレッジ > 85%
   - メモリリークゼロ

3. **パフォーマンス**
   - パフォーマンス目標を達成

4. **ツール**
   - IRダンプ機能が動作
   - GraphViz可視化が動作

5. **ドキュメント**
   - 全ての仕様書が完成
   - APIドキュメントが完成

---

## 次のバージョン（v0.15.0）

v0.14.0完了後、v0.15.0では以下の機能を実装します：

1. 定数畳み込み（Constant Folding）
2. 定数伝播（Constant Propagation）
3. デッドコード除去（Dead Code Elimination）
4. 共通部分式除去（Common Subexpression Elimination）
5. 強度削減（Strength Reduction）
6. ループ不変式の移動（Loop-Invariant Code Motion）

---

## 参考資料

### コンパイラ設計
- [Rust Compiler Development Guide](https://rustc-dev-guide.rust-lang.org/)
- [LLVM Documentation](https://llvm.org/docs/)
- Modern Compiler Implementation (Tiger Book)
- Engineering a Compiler (2nd Edition)

### SSA形式
- Cytron et al., "Efficiently Computing Static Single Assignment Form and the Control Dependence Graph" (1991)
- Braun et al., "Simple and Efficient Construction of Static Single Assignment Form" (2013)

### データフロー解析
- Aho et al., "Compilers: Principles, Techniques, and Tools" (Dragon Book)
- Muchnick, "Advanced Compiler Design and Implementation"

---

## まとめ

v0.14.0では、Cb言語のネイティブコンパイラ実装の基盤となる3層IR構造（HIR/MIR/LIR）を設計・実装します。これにより：

1. **型情報の完全な解決**（HIR）
2. **最適化に適した表現**（MIR with SSA）
3. **コード生成への準備**（LIR）

が実現され、v0.15.0以降の最適化とコード生成の実装が可能になります。

**実装期間**: 3-4ヶ月
**主要成果物**: HIR/MIR/LIR実装、IRビューワー、データフロー解析基盤
**目標**: v0.15.0以降での最適化実装への準備完了
