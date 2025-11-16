# v0.15.0 - IR基盤とコンパイラ最適化

**バージョン**: v0.15.0
**目標**: 3層IR構造（HIR/MIR/LIR）の実装とコンパイラ最適化基盤の構築
**期間**: 3-4ヶ月
**ステータス**: 計画中
**作成日**: 2025年11月14日

---

## 🎯 概要

v0.15.0は、Cb言語のネイティブコンパイラ実装における最初の大きなマイルストーンです。このバージョンでは、コンパイラの中間表現（IR）を3層構造（HIR/MIR/LIR）で設計・実装し、将来の最適化とコード生成への基盤を構築します。

**注**: 旧ロードマップのv0.16.0のIR部分を前倒しして実装します。

### 主要目標

1. **3層IR構造の完全実装**（HIR/MIR/LIR）
2. **SSA形式とデータフロー解析**
3. **基本的な最適化パス**
4. **IRビューワーと可視化ツール**

---

## 📊 v0.14.0からの変更点

### v0.14.0で達成されること

- ✅ Integration testカバレッジ100%
- ✅ Async機能の成熟（関数型、ラムダ式、構造体メソッド）
- ✅ ジェネリクスの成熟（Generic Interface impl構文）
- ✅ パターンマッチングの強化

### v0.15.0で新たに実装する機能

- 🆕 **HIR (High-level IR)**: ASTに型情報とスコープ情報を付加
- 🆕 **MIR (Mid-level IR)**: SSA形式、CFG、データフロー解析
- 🆕 **LIR (Low-level IR)**: 3アドレスコード、仮想レジスタ管理
- 🆕 **IRツール**: ダンプ機能、CFG可視化、IR検証

---

## 📋 主要な成果物

### 1. IR（中間表現）層

#### HIR (High-level IR) - 高レベル中間表現

**役割**: ASTと型情報を統合した高レベル表現

**特徴**:
- ASTに型情報とスコープ情報を付加
- 制御フロー構造を保持（if/while/for等）
- ジェネリクスの単相化（Monomorphization）
- 型推論の実行

**実装ファイル**:
- `src/backend/ir/hir/hir_node.h/cpp` - HIRノード定義
- `src/backend/ir/hir/hir_generator.h/cpp` - ASTからHIRへの変換
- `src/backend/ir/hir/hir_visitor.h` - HIRビジター
- `src/backend/ir/hir/hir_dumper.h/cpp` - HIRダンプ機能

---

#### MIR (Mid-level IR) - 中レベル中間表現

**役割**: 最適化に適した中間表現

**特徴**:
- **SSA形式（Static Single Assignment）**: 各変数は一度だけ代入される
- **制御フローグラフ（CFG）**: 基本ブロックとその接続関係
- **基本ブロック（Basic Blocks）**: 分岐のない命令列
- **データフロー解析**: 生存変数解析、到達定義解析、使用-定義チェーン
- **支配木（Dominator Tree）**: Lengauer-Tarjanアルゴリズム

**実装ファイル**:
- `src/backend/ir/mir/mir_node.h/cpp` - MIRノード定義
- `src/backend/ir/mir/mir_generator.h/cpp` - HIRからMIRへの変換
- `src/backend/ir/mir/cfg.h/cpp` - 制御フローグラフ
- `src/backend/ir/mir/ssa_builder.h/cpp` - SSA形式構築
- `src/backend/ir/mir/dominator_tree.h/cpp` - 支配木
- `src/backend/ir/mir/mir_dumper.h/cpp` - MIRダンプ機能
- `src/backend/ir/mir/graphviz_gen.h/cpp` - GraphViz可視化

---

#### LIR (Low-level IR) - 低レベル中間表現

**役割**: コード生成への準備

**特徴**:
- **3アドレスコード形式**: `%1 = add %2, %3`
- **ターゲット非依存の低レベル命令**: プラットフォーム固有の詳細を隠蔽
- **仮想レジスタの管理**: 無限の仮想レジスタ
- **アセンブリ生成への準備**: v0.16.0でのバックエンド実装に備える

**実装ファイル**:
- `src/backend/ir/lir/lir_node.h/cpp` - LIRノード定義
- `src/backend/ir/lir/lir_generator.h/cpp` - MIRからLIRへの変換
- `src/backend/ir/lir/lir_dumper.h/cpp` - LIRダンプ機能

---

### 2. データフロー解析基盤

**実装する解析**:

1. **生存変数解析（Liveness Analysis）**
   - どの変数がどの時点で生存しているかを追跡
   - レジスタ割り当てに必要

2. **到達定義解析（Reaching Definitions）**
   - どの定義がどのプログラムポイントに到達するかを追跡
   - 定数伝播に必要

3. **使用-定義チェーン（Use-Def Chain）**
   - 各変数の使用箇所と定義箇所の関係を追跡
   - 最適化に必要

4. **支配木（Dominator Tree）**
   - CFG内の支配関係を表現
   - SSA構築に必要

**実装ファイル**:
- `src/backend/ir/analysis/liveness.h/cpp` - 生存変数解析
- `src/backend/ir/analysis/reaching_defs.h/cpp` - 到達定義解析
- `src/backend/ir/analysis/use_def_chain.h/cpp` - 使用-定義チェーン

---

### 3. IRビューワーとデバッグツール

**機能**:

- **HIR/MIR/LIRのテキストダンプ**: `--dump-hir`, `--dump-mir`, `--dump-lir`
- **GraphVizによるCFG可視化**: `--emit-cfg-dot`
- **支配木の可視化**: CFGと重ねて表示
- **IR検証ツール**: IRの整合性をチェック

**使用例**:
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

## 📅 実装スケジュール

### Month 1: HIR実装

**Week 1-2: HIR基本構造**
- [ ] HIRノード定義（式、文、宣言）
- [ ] ASTからHIRへの変換器
- [ ] 型情報の統合（型チェック結果の保持）
- [ ] 基本的なHIRダンプ機能
- [ ] ユニットテスト（10個）

**Week 3: HIR高度な機能**
- [ ] 制御フロー変換（if/while/for → HIR）
- [ ] 関数・構造体の変換
- [ ] スコープ情報の管理
- [ ] ユニットテスト（10個）

**Week 4: HIRジェネリクスとテスト**
- [ ] ジェネリクス単相化（`Vec<int>` → `Vec_int`）
- [ ] 型パラメータの具体化
- [ ] HIRダンプ機能の完成
- [ ] 統合テスト（5個）
- [ ] ユニットテスト（10個）

**Month 1の成果**:
- HIR完全実装
- 30ユニットテスト + 5統合テスト成功

---

### Month 2: MIR実装

**Week 1: MIR基本構造とCFG**
- [ ] MIRノード定義（基本ブロック、命令）
- [ ] CFG構築（基本ブロックの生成と接続）
- [ ] HIRからMIRへの変換器（基本部分）
- [ ] ユニットテスト（10個）

**Week 2: SSA形式**
- [ ] 支配木構築（Lengauer-Tarjanアルゴリズム）
- [ ] PHIノード挿入（支配フロンティア計算）
- [ ] 変数リネーミング（SSA変換の完成）
- [ ] ユニットテスト（15個）

**Week 3: データフロー解析**
- [ ] 生存変数解析
- [ ] 到達定義解析
- [ ] 使用-定義チェーン
- [ ] ユニットテスト（15個）

**Week 4: MIR完成とテスト**
- [ ] GraphViz可視化
- [ ] MIRダンプ機能の完成
- [ ] 統合テスト（10個）
- [ ] パフォーマンステスト

**Month 2の成果**:
- MIR完全実装
- SSA形式とデータフロー解析
- 40ユニットテスト + 10統合テスト成功

---

### Month 3: LIR実装とツール

**Week 1-2: LIR実装**
- [ ] LIRノード定義（3アドレスコード命令）
- [ ] 仮想レジスタ管理
- [ ] MIRからLIRへの変換器
- [ ] LIRダンプ機能
- [ ] ユニットテスト（20個）

**Week 3: IRビューワーとツール**
- [ ] 統一されたダンプ機能（`--dump-all-ir`）
- [ ] GraphViz可視化の改善
- [ ] IR検証ツール（整合性チェック）
- [ ] ユニットテスト（10個）

**Week 4: 統合とドキュメント**
- [ ] 統合テスト（5個）
- [ ] パフォーマンステスト
- [ ] ドキュメント完成
  - [ ] `docs/ir/HIR_SPECIFICATION.md`
  - [ ] `docs/ir/MIR_SPECIFICATION.md`
  - [ ] `docs/ir/LIR_SPECIFICATION.md`
  - [ ] `docs/ir/SSA_GUIDE.md`
  - [ ] `docs/ir/DATAFLOW_ANALYSIS.md`
- [ ] リリース準備

**Month 3の成果**:
- LIR完全実装
- IRツール完成
- 30ユニットテスト + 5統合テスト成功
- 完全なドキュメント

---

### Month 4: 最適化パス（オプショナル）

**Week 1-2: 基本的な最適化**
- [ ] デッドコード除去（Dead Code Elimination）
- [ ] 定数畳み込み（Constant Folding）
- [ ] 定数伝播（Constant Propagation）
- [ ] ユニットテスト（15個）

**Week 3-4: 高度な最適化とテスト**
- [ ] 共通部分式除去（Common Subexpression Elimination）
- [ ] 強度削減（Strength Reduction）
- [ ] 統合テスト（5個）
- [ ] パフォーマンスベンチマーク

**Month 4の成果**:
- 基本的な最適化パス実装
- 15ユニットテスト + 5統合テスト成功

---

## 🧪 テスト

### ユニットテスト（合計155個以上）

**HIR（30個）**:
- ASTからHIRへの変換（10個）
- 型情報の統合（10個）
- ジェネリクス単相化（10個）

**MIR（40個）**:
- CFG構築（10個）
- SSA構築（15個）
- データフロー解析（15個）

**LIR（30個）**:
- MIRからLIRへの変換（20個）
- 仮想レジスタ管理（10個）

**最適化（15個）** ※Month 4
- デッドコード除去（5個）
- 定数畳み込み・伝播（5個）
- 共通部分式除去（5個）

**IRツール（10個）**:
- ダンプ機能（5個）
- 可視化機能（5個）

---

### 統合テスト（合計25個以上）

**IRラウンドトリップ（20個）**:
- AST → HIR → MIR → LIR の変換が正しく動作することを確認
- 簡単な関数（5個）
- 制御フロー（5個）
- ネストループ（5個）
- ジェネリクス（5個）

**IRダンプ検証（5個）**:
- ダンプされたIRが正しい形式であることを確認

---

### ベンチマーク

**パフォーマンス目標**:
- **HIR生成**: 1000行のコードを100ms以内で処理
- **MIR生成**: HIRから50ms以内で変換
- **LIR生成**: MIRから30ms以内で変換
- **メモリ使用量**: 1000行のコードで50MB以内

---

## 📊 プロジェクト構造

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

docs/ir/                       # IRドキュメント
├── HIR_SPECIFICATION.md
├── MIR_SPECIFICATION.md
├── LIR_SPECIFICATION.md
├── SSA_GUIDE.md
└── DATAFLOW_ANALYSIS.md
```

---

## 💡 実装例

### 例1: シンプルな関数

**ソースコード**:
```cb
int add(int x, int y) {
    return x + y;
}
```

**HIRダンプ**:
```
function int add(x: int, y: int) {
    return BinaryOp(+, Var(x: int), Var(y: int));
}
```

**MIRダンプ（SSA形式）**:
```
function int add(_0: int, _1: int) {
    let mut _2: int;  // return value
    let mut _3: int;  // temp

    bb0: {
        _3 = Add(_0, _1);
        _2 = _3;
        return _2;
    }
}
```

**LIRダンプ（3アドレスコード）**:
```
add:
    ; %0 = x (parameter)
    ; %1 = y (parameter)
    %2 = Add %0, %1
    Return %2
```

---

### 例2: 制御フロー

**ソースコード**:
```cb
int max(int a, int b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}
```

**MIRダンプ（CFG）**:
```
function int max(_0: int, _1: int) {
    let mut _2: int;  // return value
    let mut _3: bool; // condition

    bb0: {
        _3 = Gt(_0, _1);
        branch _3, bb1, bb2;
    }

    bb1: {  // then branch
        _2 = _0;
        return _2;
    }

    bb2: {  // else branch
        _2 = _1;
        return _2;
    }
}
```

**CFG可視化（GraphViz）**:
```
digraph max {
    bb0 [label="bb0\n_3 = Gt(_0, _1)\nbranch _3"];
    bb1 [label="bb1\n_2 = _0\nreturn _2"];
    bb2 [label="bb2\n_2 = _1\nreturn _2"];

    bb0 -> bb1 [label="true"];
    bb0 -> bb2 [label="false"];
}
```

---

## 🔧 技術スタック

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

## ✅ 完了条件

v0.15.0は以下の条件を満たしたときに完了とします：

### 必須条件（Must Have）

- [ ] **HIR/MIR/LIRの全実装完了**
- [ ] **ASTからLIRまでの完全な変換パイプライン**
- [ ] **SSA形式の正しい実装**
- [ ] **全ユニットテスト成功**（155個以上）
- [ ] **全統合テスト成功**（25個以上）
- [ ] **コードカバレッジ > 85%**
- [ ] **メモリリークゼロ**

### 推奨条件（Should Have）

- [ ] **IRダンプ機能が動作**
- [ ] **GraphViz可視化が動作**
- [ ] **パフォーマンス目標を達成**
  - HIR生成: 1000行 < 100ms
  - MIR生成: < 50ms
  - LIR生成: < 30ms
  - メモリ: < 50MB

### 任意条件（Nice to Have）（Month 4）

- [ ] 基本的な最適化パス実装
- [ ] デッドコード除去
- [ ] 定数畳み込み・伝播

---

## 🔗 関連ドキュメント

- **v0.14.0実装計画**: [`../v0.14.0/v0.14.0_implementation_plan.md`](../v0.14.0/v0.14.0_implementation_plan.md)
- **v0.16.0実装計画**: [`../v0.16.0/README.md`](../v0.16.0/README.md) - 複数バックエンド対応
- **総合ロードマップ**: [`../ROADMAP_v0.14-v0.18_SUMMARY.md`](../ROADMAP_v0.14-v0.18_SUMMARY.md)

---

## 📚 参考資料

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

## 📝 注意事項

### 後方互換性

- v0.14.0のコードは全て動作する必要がある
- 既存のインタプリタは維持（`--interpret`フラグで実行可能）
- IR実装は既存の動作に影響を与えない

### 段階的な実装

- IR実装は段階的に行う（HIR → MIR → LIR）
- 各段階でテストを実施し、動作を確認
- Month 4の最適化パスはオプショナル

### v0.16.0への準備

- LIRは v0.16.0 のバックエンド実装に備えて設計される
- v0.16.0 でネイティブコード生成、WASM生成を実装
- IR基盤があることで、バックエンド追加が容易になる

---

**作成者**: Cb Language Team
**最終更新**: 2025年11月14日
**ステータス**: Planning

---

## 🎯 v0.15.0のハイライト

1. **3層IR構造の完成** - HIR/MIR/LIRの完全実装
2. **SSA形式** - 最適化に適した中間表現
3. **データフロー解析基盤** - 将来の最適化に備える
4. **IRビューワー** - 開発者がIRを視覚的に理解できる

v0.15.0の完成により、v0.16.0でのネイティブコード生成、v0.17.0での最適化実装の準備が整います。
