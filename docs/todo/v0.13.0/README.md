# v0.13.0 実装計画

**バージョン**: v0.13.0
**作成日**: 2025-11-13
**ステータス**: 設計完了・実装準備中

## 概要

v0.13.0では、以下の3つの主要機能を実装します：

1. **FFI (Foreign Function Interface)** - 外部ライブラリとの連携
2. **プリプロセッサ** - 条件付きコンパイル
3. **C風マクロ** - シンプルなテキスト置換

## ドキュメント構成

| ファイル | 内容 | ステータス |
|---------|------|----------|
| `README.md` | このファイル | ✅ |
| `version_roadmap.md` | バージョン戦略とロードマップ | ✅ |
| `modern_ffi_macro_design.md` | FFI・マクロ・プリプロセッサの設計 | ✅ |
| `inline_asm_cpp_feasibility.md` | インラインasm/cppの実現可能性調査 | ✅ |

## v0.13.0 の主要機能

### 1. FFI (Foreign Function Interface)

**構文**:
```cb
use foreign.math {
    int add(int a, int b);
    double sqrt(double x);
}

void main() {
    int x = math.add(10, 20);
    println(x);
}
```

**特徴**:
- ✅ 既存のCb関数定義と同じ形式
- ✅ C ABI互換の任意の言語に対応（C, C++, Rust, Zig, Go, etc.）
- ✅ importと一貫性のある構文
- ✅ 型安全

**対応ファイル**:
- `.cbf` - 型定義ファイル
- `.so`, `.dylib`, `.dll` - 共有ライブラリ

### 2. プリプロセッサ

**構文**:
```cb
#define DEBUG
#define MAX_BUFFER_SIZE 1024

#ifdef DEBUG
    void log(string msg) {
        println("[DEBUG]", msg);
    }
#else
    void log(string msg) { }
#endif
```

**サポートするディレクティブ**:
- `#define` - マクロ定義
- `#ifdef`, `#ifndef` - 条件チェック
- `#elseif`, `#else`, `#endif` - 条件分岐
- `#error`, `#warning` - エラー/警告
- `#undef` - マクロ削除

**組み込みマクロ**:
- `__FILE__` - ファイル名
- `__LINE__` - 行番号
- `__DATE__` - コンパイル日付
- `__TIME__` - コンパイル時刻
- `__VERSION__` - Cbバージョン

### 3. C風マクロ

**構文**:
```cb
// 定数マクロ
#define PI 3.14159
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// 可変長引数マクロ
#define LOG(level, ...) \
    println("[" + level + "]", __VA_ARGS__)

void main() {
    int max_val = MAX(10, 20);
    LOG("INFO", "Server started");
}
```

**特徴**:
- ✅ C/C++と同じシンプルなマクロ
- ✅ 複数行マクロ対応（バックスラッシュ継続）
- ✅ 可変長引数対応（`__VA_ARGS__`）
- ❌ Rust風の複雑なマクロは不採用

**代替手段**:
- ジェネリクス（すでに実装済み）
- inline関数（将来）
- constexpr（将来）

## 実装しない機能

### インラインアセンブリ (`asm("")`)

**理由**:
- インタプリタでは技術的に困難
- 巨大な依存関係（LLVM JIT等）が必要
- FFIで代替可能

**将来の実装**:
- v1.0.0（ネイティブコンパイラ）で再検討

### インラインC++ (`cpp("")`)

**理由**:
- 動的コンパイルのオーバーヘッドが大きい
- FFIの方が実用的

**代替手段**:
- FFI経由でC++ライブラリを使用

## 実装タイムライン

### Phase 1: プリプロセッサ基盤（Week 1-2）
- [ ] Lexerの拡張（#で始まるトークン）
- [ ] プリプロセッサディレクティブのパース
- [ ] マクロ展開エンジン
- [ ] 組み込みマクロ

### Phase 2: FFI基盤（Week 3-4）
- [ ] `use foreign` 構文のパース
- [ ] .cbfファイルのパース
- [ ] dlopen/dlsym ラッパー
- [ ] 基本的な型変換

### Phase 3: FFI拡張機能（Week 5-6）
- [ ] 構造体の受け渡し
- [ ] ポインタ型のサポート
- [ ] 可変長引数のサポート
- [ ] コールバック関数

### Phase 4: プリプロセッサ拡張（Week 7）
- [ ] #undef
- [ ] #error / #warning
- [ ] 可変長引数マクロ
- [ ] 複数行マクロ

### Phase 5: 統合とテスト（Week 8）
- [ ] 統合テスト
- [ ] ドキュメント作成
- [ ] サンプルコード作成

## 設計原則

1. **型安全性** - コンパイル時の型チェック
2. **既存構文との一貫性** - importと同じパターン
3. **シンプルさ** - 複雑な機能は避ける
4. **実用性** - 実際に使える機能を優先

## 参考資料

- `version_roadmap.md` - 全体のバージョン戦略
- `modern_ffi_macro_design.md` - FFI/マクロ/プリプロセッサの詳細設計
- `inline_asm_cpp_feasibility.md` - インラインasm/cppの技術調査

## 次のステップ

1. Phase 1の実装開始（プリプロセッサ）
2. プロトタイプのテスト
3. ユーザーフィードバックの収集
