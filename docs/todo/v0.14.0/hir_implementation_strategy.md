# HIR実装戦略

## 概要

CbのHIR（High-level Intermediate Representation）を実装し、実行可能なバイナリを生成するための現実的な戦略を検討します。

## 選択肢

### 選択肢1: HIR → C++トランスパイル（最も現実的）⭐

**メリット:**
- ✅ 実装が最も簡単で高速
- ✅ 既存のC++コンパイラ（gcc/clang）の最適化を利用できる
- ✅ デバッグが容易
- ✅ 段階的な実装が可能
- ✅ クロスプラットフォーム対応が簡単

**デメリット:**
- ❌ 中間コンパイル時間がかかる（C++コンパイル）
- ❌ 独自の最適化が制限される

**実装フロー:**
```
AST → HIR → C++コード → gcc/clang → 実行可能バイナリ
            ↓
          MIR（最適化）
            ↓
          C++コード
```

**推奨理由:**
- Cbの現在のアーキテクチャと相性が良い
- 既存のC++生成機能を活用できる
- HIR/MIRで最適化を行った後、C++に変換するだけ

---

### 選択肢2: HIR → LLVM IR

**メリット:**
- ✅ 強力な最適化パス
- ✅ 多くの言語で採用されている
- ✅ クロスプラットフォーム対応

**デメリット:**
- ❌ LLVM依存が必要（大きなバイナリサイズ）
- ❌ 実装が複雑
- ❌ デバッグが困難

**実装フロー:**
```
AST → HIR → MIR → LLVM IR → llc → 実行可能バイナリ
```

---

### 選択肢3: HIR → 独自VM

**メリット:**
- ✅ 完全なコントロール
- ✅ デバッグ機能を自由に追加できる
- ✅ 最適化を自由に実装できる

**デメリット:**
- ❌ 実装コストが非常に高い
- ❌ パフォーマンスが劣る可能性
- ❌ メンテナンスコストが高い

**実装フロー:**
```
AST → HIR → MIR → LIR → バイトコード → VM → 実行
```

---

### 選択肢4: HIR → アセンブリ直接生成

**メリット:**
- ✅ 完全なコントロール
- ✅ 最高のパフォーマンス

**デメリット:**
- ❌ 実装コストが極めて高い
- ❌ プラットフォーム依存（x86, ARM等）
- ❌ デバッグが極めて困難

---

## 推奨戦略: ハイブリッドアプローチ

### フェーズ1: HIR → C++トランスパイル（短期）

```
AST → HIR → C++コード → gcc/clang → バイナリ
```

**実装ステップ:**

1. **HIRの完全実装**
   - すべてのCb機能をHIRで表現
   - HIRビルダーパターンの実装
   - HIR検証器の実装

2. **HIR → C++バックエンド**
   - `HIRToCppTranspiler`クラスの実装
   - HIR各ノードをC++コードに変換
   - 既存のC++生成ロジックを活用

3. **最適化パスの追加（オプショナル）**
   - デッドコード削除
   - 定数畳み込み
   - インライン展開

### フェーズ2: MIRの導入（中期）

```
AST → HIR → MIR（最適化）→ C++コード → バイナリ
```

**MIRの役割:**
- HIRよりも低レベルな表現
- SSA形式（Static Single Assignment）
- より高度な最適化（ループ最適化、レジスタ割り当てヒント等）

### フェーズ3: LLVMバックエンド（長期・オプショナル）

```
AST → HIR → MIR → LLVM IR → バイナリ
                ↓
              C++コード（デバッグ用）
```

- より高度な最適化が必要な場合のみ
- C++バックエンドは残す（デバッグ・互換性用）

---

## 具体的な実装計画

### ステップ1: HIRの完全実装（1-2週間）

**目標:** すべてのCb機能をHIRで表現できるようにする

```cpp
// src/backend/ir/hir/
hir_node.h              // HIRノード定義（完全版）
hir_builder.h           // HIRビルダー
hir_generator.h/cpp     // AST→HIR変換（拡張）
hir_validator.h/cpp     // HIR検証
hir_optimizer.h/cpp     // 基本最適化
hir_printer.h/cpp       // HIRダンプ（デバッグ用）
```

**実装する機能:**
- [x] 基本型（int, string, etc.）
- [ ] 関数（通常、ジェネリック、ラムダ）
- [ ] 制御フロー（if, for, while, switch, match）
- [ ] 構造体・Enum
- [ ] インターフェース・Impl
- [ ] 配列・ポインタ
- [ ] 演算子（算術、論理、ビット）
- [ ] メモリ管理（new, delete, defer）
- [ ] Async/Await
- [ ] エラーハンドリング（try, catch, ?演算子）
- [ ] FFI

### ステップ2: HIR → C++バックエンド（1週間）

```cpp
// src/backend/codegen/
hir_to_cpp.h/cpp        // HIR→C++変換
```

**変換例:**

```cb
// Cb
fn add(a: int, b: int): int {
    return a + b;
}
```

```
// HIR
HIRFunction {
    name: "add",
    params: [(a, int), (b, int)],
    return_type: int,
    body: HIRReturn(HIRBinaryOp("+", HIRVar("a"), HIRVar("b")))
}
```

```cpp
// C++ (生成)
int add(int a, int b) {
    return a + b;
}
```

### ステップ3: 統合とテスト（1週間）

1. **ユニットテスト**
   - `tests/unit/hir/` - HIR生成テスト
   - `tests/unit/mir/` - MIR最適化テスト（将来）
   - `tests/unit/backend/` - コード生成テスト

2. **統合テスト**
   - `tests/integration/` - すべてのCb機能が動作することを確認

3. **パフォーマンステスト**
   - 既存のインタプリタと比較
   - コンパイル時間の測定

---

## 実装の優先順位

### 最優先（コア機能）
1. 基本型と変数
2. 関数（通常）
3. 制御フロー（if, for, while）
4. 算術演算
5. 構造体
6. 配列

### 高優先
7. ポインタ・参照
8. インターフェース・Impl
9. Enum
10. ジェネリクス

### 中優先
11. ラムダ
12. Async/Await
13. エラーハンドリング
14. パターンマッチング

### 低優先
15. FFI（既存の仕組みを活用）
16. プリプロセッサ（既存の仕組みを活用）

---

## ディレクトリ構造

```
src/backend/
├── ir/
│   ├── common/           # IR共通
│   │   ├── ir_types.h
│   │   └── ir_common.h
│   ├── hir/              # High-level IR
│   │   ├── hir_node.h
│   │   ├── hir_builder.h
│   │   ├── hir_generator.h/cpp
│   │   ├── hir_validator.h/cpp
│   │   ├── hir_optimizer.h/cpp
│   │   └── hir_printer.h/cpp
│   ├── mir/              # Mid-level IR（将来）
│   │   ├── mir_node.h
│   │   ├── mir_optimizer.h/cpp
│   │   └── mir_printer.h/cpp
│   └── lir/              # Low-level IR（将来）
│       └── lir_node.h
└── codegen/              # コード生成
    ├── hir_to_cpp.h/cpp  # HIR→C++
    ├── hir_to_llvm.h/cpp # HIR→LLVM（将来）
    └── cpp_emitter.h/cpp # C++出力ヘルパー

tests/
├── unit/
│   ├── hir/              # HIRユニットテスト
│   ├── mir/              # MIRユニットテスト
│   └── backend/          # バックエンドテスト
└── integration/          # 統合テスト（既存）
```

---

## まとめ

### 推奨アプローチ: HIR → C++トランスパイル

**理由:**
1. ✅ 実装コストが低い
2. ✅ 既存の資産を活用できる
3. ✅ デバッグが容易
4. ✅ 段階的に改善できる（MIR、LLVMへの移行が可能）
5. ✅ クロスプラットフォーム対応が簡単

**タイムライン:**
- Week 1-2: HIR完全実装
- Week 3: HIR → C++バックエンド
- Week 4: テストと統合

このアプローチにより、約1ヶ月で実行可能なHIRベースのコンパイラを実装できます。
