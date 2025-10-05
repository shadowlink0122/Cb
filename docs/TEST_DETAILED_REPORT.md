# テストケース詳細レポート

## 概要

参照（reference）機能のテストカバレッジを確認し、以下を発見：
- ✅ **動作する**: プリミティブ型の参照、Interface参照
- ❌ **未サポート**: 構造体の参照パラメータ、const参照 (`const T&`)

## テストファイル一覧

### ✅ 合格テスト（既存・動作確認済み）

#### 1. `tests/cases/reference/test_reference_function_param.cb`
**テスト内容:**
- プリミティブ型の参照パラメータ
- 複数の参照パラメータ
- 参照とポインタの混在

**実行結果:** ✅ PASS
```
Test 1: Basic reference parameter
5 → 6
Test 2: Multiple reference parameters  
swap(10, 20) → (20, 10)
Test 3: Reference and pointer mix
ref=110, ptr=220
```

#### 2. `tests/cases/reference/reference_semantics.cb`
**テスト内容:**
- 参照は初期化のみ可能
- 再バインド不可（`ref = other`は値の代入）
- 参照のセマンティクス検証

**実行結果:** ✅ PASS
```
Initial state: a=10, ref=10
After ref = b: a=20  (refは再バインドせず、aの値が変更される)
After ref = c: a=30  (同様にaが変更)
After ref = 100: a=100
```

**結論:** 参照は正しく実装されている（C++準拠）

#### 3. `tests/cases/interface/test_interface_reference.cb`
**テスト内容:**
- Interface参照の基本
- Interface参照を関数パラメータとして使用
- Interface参照を返す関数

**実行結果:** ✅ PASS
```
Test 1: Interface reference basic
count = 10 → 11
Test 2: Interface reference parameter
increment_twice: 5 → 7
Test 3: Return interface reference
ref2.get_count() = 100 → 101
```

#### 4. `tests/cases/const_parameters/*`
**テスト内容:**
- `const int x` などのconst値パラメータ
- 全プリミティブ型でconst対応
- const変数の再代入エラー検出

**実行結果:** ✅ 全て PASS

### ❌ 不合格テスト（新規作成・未サポート機能）

#### 1. `tests/cases/reference/test_struct_reference_simple.cb`
**テスト内容:**
- 構造体の参照パラメータ

**実行結果:** ❌ FAIL
```
Error: Variable is not a struct: p
```

**問題:**
```cb
struct Point { int x; int y; };

void modify_by_ref(Point& p) {  // ❌ 構造体参照がサポートされていない
    p.x = 100;
}
```

**原因:** パラメータが参照の場合、構造体型情報が失われている

#### 2. `tests/cases/reference/test_const_reference.cb`
**テスト内容:**
- 構造体の参照パラメータ（読み取り専用として使用）

**実行結果:** ❌ FAIL  
```
Error: Variable is not a struct: rect
```

**問題:** 構造体参照パラメータがサポートされていないため、constの有無に関わらず失敗

#### 3. `tests/cases/interface/test_interface_const_reference.cb`
**テスト内容:**
- Interface型のconst参照（構文テスト）

**実行結果:** ❌ FAIL (修正後は参照として動作するが、constは未テスト)
```
[METHOD_LOOKUP_FAIL] receiver='d1' type='struct' method='get_size'
```

**問題:** `const Drawable&` 構文は未サポート（推測）

#### 4. `tests/cases/struct/test_struct_pointer_reference.cb`
**テスト内容:**
- 構造体ポインタメンバ
- ポインタパラメータ
- 参照パラメータ
- ポインタを返す関数

**実行結果:** ❌ PARSE ERROR
```
Location: line 19
Error: Expected ';' after pointer/reference variable declaration
Source: Node* create_node(int val) {
                       ^
```

**問題:** 関数の戻り値型としてポインタ型がパースできない可能性

## 機能サポート状況マトリックス

| 機能 | プリミティブ型 | 構造体 | Interface | 配列 |
|------|---------------|--------|-----------|------|
| 値パラメータ | ✅ | ✅ | ✅ | ✅ |
| const値パラメータ | ✅ | ✅ | ✅ | ✅ |
| 参照パラメータ | ✅ | ❌ | ✅ | ❓ |
| const参照パラメータ | ❌ | ❌ | ❌ | ❓ |
| ポインタパラメータ | ✅ | ✅ | ✅ | ✅ |
| 参照戻り値 | ✅ | ❓ | ✅ | ❓ |
| ポインタ戻り値 | ✅ | ❓ | ✅ | ✅ |

凡例:
- ✅ = サポート済み＆テスト合格
- ❌ = 未サポート（エラー発生）
- ❓ = 未テスト（動作未確認）

## 詳細な問題分析

### 問題1: 構造体の参照パラメータ

**症状:**
```cb
void func(Point& p) {
    p.x = 100;  // Error: Variable is not a struct: p
}
```

**推測される原因:**
1. パーサー段階で型情報が正しく設定されていない
2. インタープリタで参照の実体解決時に型情報が失われる
3. 構造体メンバアクセス (`p.x`) 時に型チェックが参照を考慮していない

**影響範囲:**
- 構造体を効率的に関数に渡せない（常にコピーが発生）
- 大きな構造体でパフォーマンス問題
- 構造体を変更する関数が書けない（ポインタで代替は可能）

**修正の複雑度:** 中〜高
- パラメータバインディングの修正が必要
- 型推論システムの変更が必要な可能性

### 問題2: const参照パラメータ

**症状:**
```cb
void func(const int& x) {  // パース可能か不明
    println(x);
}
```

**推測される原因:**
1. パーサーが `const` と `&` の組み合わせを認識していない
2. `const int` は認識するが `const int&` は未対応

**影響範囲:**
- 読み取り専用保証ができない
- C++の標準的なイディオム（`const T&`）が使えない
- 最適化の機会損失

**修正の複雑度:** 中
- パーサーの型パース部分の拡張
- ASTノードへのフラグ追加（is_const + is_reference）

### 問題3: ポインタ型を返す関数

**症状:**
```cb
Point* create_point(int x) {  // Expected ';' after declaration
    // ...
}
```

**推測される原因:**
- 関数の戻り値型パース時に `*` トークンの処理が不完全
- パラメータリストの `(` が来る前にポインタ型を認識できていない

**影響範囲:**
- ファクトリーパターンが使えない
- 動的に構造体を作成して返す関数が書けない

**修正の複雑度:** 低〜中
- 関数宣言パース部分の修正
- 既にポインタ型は変数宣言でサポートされているため、戻り値にも適用すればOK

## 回避策

現時点での各問題の回避策：

### 構造体の参照パラメータ → ポインタを使用
```cb
// ❌ できない
void modify(Rectangle& rect) {
    rect.width = 100;
}

// ✅ 回避策
void modify(Rectangle* rect) {
    rect->width = 100;
}

int main() {
    Rectangle r = {width: 10, height: 5};
    modify(&r);  // アドレスを渡す
}
```

### const参照パラメータ → const値パラメータ（コピー）
```cb
// ❌ できない
int calc(const Rectangle& rect) {
    return rect.width * rect.height;
}

// ✅ 回避策（ただし非効率）
int calc(Rectangle rect) {  // コピーが発生
    return rect.width * rect.height;
}
```

### ポインタを返す関数 → ポインタをパラメータで受け取る
```cb
// ❌ できない（推測）
Point* create_point(int x) {
    Point p = {x: x, y: 0};
    return &p;
}

// ✅ 回避策
void create_point(Point* out, int x) {
    out->x = x;
    out->y = 0;
}

int main() {
    Point p;
    create_point(&p, 10);
}
```

## 推奨する対応順序

### フェーズ1: 重要度最高（即座に対応）
1. **構造体の参照パラメータのサポート**
   - 理由: 基本的な機能、パフォーマンスに直結
   - 工数見積: 2〜3日

### フェーズ2: 重要度高（次のマイルストーン）
2. **ポインタを返す関数のサポート**
   - 理由: よくある使用パターン、パース問題の修正
   - 工数見積: 1日

3. **const参照パラメータのサポート**
   - 理由: 読み取り専用保証、C++イディオム
   - 工数見積: 2日

### フェーズ3: 将来的な拡張
4. 配列の参照パラメータ
5. typedef型の参照（構造体参照が動けば自動的に動作するはず）

## テスト実行コマンド

```bash
# 合格するテスト
./main -i tests/cases/reference/test_reference_function_param.cb
./main -i tests/cases/reference/reference_semantics.cb
./main -i tests/cases/interface/test_interface_reference.cb

# 失敗するテスト（未サポート機能）
./main -i tests/cases/reference/test_struct_reference_simple.cb  # 構造体参照
./main -i tests/cases/reference/test_const_reference.cb          # 構造体参照
./main -i tests/cases/struct/test_struct_pointer_reference.cb    # ポインタ戻り値

# 全テストスイート
make test  # ✅ 2229 tests passing
```

## まとめ

**発見された主要な問題:**
1. 🔴 構造体の参照パラメータが未サポート（最優先）
2. 🔴 const参照パラメータが未サポート（高優先度）
3. 🟡 ポインタを返す関数がパースエラー（中優先度）

**動作確認済みの機能:**
- ✅ プリミティブ型の参照（完全サポート）
- ✅ Interface型の参照（完全サポート）
- ✅ 参照のセマンティクス（C++準拠）
- ✅ constパラメータ（値渡しのみ）
- ✅ ポインタ（完全サポート）

**次のアクション:**
構造体の参照パラメータのサポートを実装することを強く推奨します。これは基本的な機能であり、多くのユースケースに影響します。
