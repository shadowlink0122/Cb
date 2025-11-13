# v0.16.0 - IR（中間表現）実装

**バージョン**: v0.16.0
**目標**: コンパイラの中間表現（IR）を設計・実装する
**期間**: 3-4ヶ月
**ステータス**: 計画中

---

## 概要

v0.16.0は、Cb言語のネイティブコンパイラ実装における最初の大きなマイルストーンです。このバージョンでは、コンパイラの中間表現（IR）を3層構造（HIR/MIR/LIR）で設計・実装し、将来の最適化とコード生成の基盤を構築します。

**このバージョンは大規模リファクタリングを含むため、マイナーバージョンとして設定されています。**

---

## 主要な成果物

### 1. HIR (High-level IR) - 高レベル中間表現
- ASTに型情報とスコープ情報を付加
- 制御フロー構造を保持（if/while/for等）
- ジェネリクスの単相化（Monomorphization）

### 2. MIR (Mid-level IR) - 中レベル中間表現
- SSA形式（Static Single Assignment）
- 制御フローグラフ（CFG）
- 基本ブロック（Basic Blocks）
- データフロー解析

### 3. LIR (Low-level IR) - 低レベル中間表現
- 3アドレスコード形式
- ターゲット非依存の低レベル命令
- 仮想レジスタの管理
- アセンブリ生成への準備

### 4. データフロー解析基盤
- 生存変数解析（Liveness Analysis）
- 到達定義解析（Reaching Definitions）
- 使用-定義チェーン（Use-Def Chain）
- 支配木（Dominator Tree）

### 5. IRビューワーとデバッグツール
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

## IR例

### ソースコード
```cb
fn add(x: int, y: int) -> int {
    return x + y;
}
```

### HIRダンプ
```
fn add(x: int, y: int) -> int {
    return BinaryOp(+, Var(x: int), Var(y: int));
}
```

### MIRダンプ
```
fn add(_0: int, _1: int) -> int {
    let mut _2: int;  // return value
    let mut _3: int;  // temp

    bb0: {
        _3 = Add(_0, _1);
        _2 = _3;
        return _2;
    }
}
```

### LIRダンプ
```
add:
    ; %0 = x (parameter)
    ; %1 = y (parameter)
    %2 = Add %0, %1
    Return %2
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

v0.16.0は以下の条件を満たしたときに完了：

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

## 次のバージョン（v0.17.0）

v0.16.0完了後、v0.17.0では以下の最適化機能を実装します：

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

v0.16.0では、Cb言語のネイティブコンパイラ実装の基盤となる3層IR構造（HIR/MIR/LIR）を設計・実装します。これにより：

1. **型情報の完全な解決**（HIR）
2. **最適化に適した表現**（MIR with SSA）
3. **コード生成への準備**（LIR）

が実現され、v0.17.0以降の最適化とコード生成の実装が可能になります。

**実装期間**: 3-4ヶ月
**主要成果物**: HIR/MIR/LIR実装、IRビューワー、データフロー解析基盤
**目標**: v0.17.0での最適化実装への準備完了
