# Cb Language Implementation Status

**最終更新**: 2025年11月16日  
**現在のバージョン**: v0.13.3  
**次期バージョン**: v0.13.4 (計画中)

---

## ✅ 実装完了機能

### v0.13.3: Documentation & Known Issues
- ✅ 既知の問題と制限事項の文書化
- ✅ テストスイートの拡張（v0.13.3テストケース追加）
- ✅ 既存機能の安定性確認（全33テスト、5スイートがパス）

### v0.13.2: Generic Array String Fix
- ✅ Generic配列での文字列型のバグ修正（`Container<string>`など）
- ✅ Asyncラムダ式の完全サポート
- ✅ 包括的なテストスイートの追加

### v0.13.1: Async Impl & Self Sync
- ✅ Interface/Implで`async`メソッドを宣言・実装でき、`await`や`yield`を含むロジックでも`self`の状態が自動で同期される。
- ✅ `SimpleEventLoop`がタスクスコープ内で`__self_receiver__`を追跡し、完了時にレシーバー構造体へ差分を書き戻す。
- ✅ Interpreter APIがイベントループとFFIマネージャーへレシーバー情報を伝播し、implメソッドから`self`経由で安全に書き込みできる。
- ✅ `test_impl_async_method*.cb`や`test_struct_async_method_basic.cb`などの新規Asyncテストが追加され、標準/ASanビルド双方で回帰テスト済み。

### v0.12.0: Async/Await基本機能
- ✅ async/await構文
- ✅ Future<T>ビルトイン型
- ✅ 非ブロッキングsleep
- ✅ yield機能
- ✅ SimpleEventLoop
- ✅ 複数Futureの順次await
- ✅ 同じFutureの複数回await
- ✅ int/string/struct型Future

### v0.12.1: Error Propagation & Timeout
- ✅ ?オペレーター（Result<T, E>とOption<T>）
- ✅ timeout()関数
- ✅ タイムアウトチェック機構
- ✅ Integration test完備

### v0.11.0: Generics & Pattern Matching
- ✅ ジェネリクス構造体（基本）
- ✅ Result<T, E>型（ビルトイン）
- ✅ Option<T>型（ビルトイン）
- ✅ match文
- ✅ Enum with Associated Values
- ✅ パターンマッチング

### v0.10.x以前
- ✅ 基本型（int, long, float, double, string, bool）
- ✅ 構造体
- ✅ インターフェース
- ✅ impl構文
- ✅ 関数
- ✅ 配列
- ✅ ポインタ
- ✅ 制御構文（if, while, for, match）

---

## ⚠️ 部分実装・制限事項

### 1. ジェネリクス関連

#### ✅ 動作する
```cb
// 基本的なジェネリクス構造体
Option<int> some_val = Option<int>::Some(42);
Result<string, int> result = Result<string, int>::Ok("success");

// ネストしたジェネリクス（スペース必須）
Option<Result<int, string> > nested;

// ジェネリックインターフェース（impl）
interface Container<T> {
    T get();
}
impl Container<int> for Box {
    int get() { return this->value; }
}

// async + ジェネリクス
async Result<int, string> process() {
    return Result<int, string>::Ok(42);
}
```

#### ❌ 未実装
```cb
// ジェネリクス構造体の配列
Future<int> futures[3];  // 宣言は可能
int r = await futures[0];  // ❌ 型情報が失われる

// >>トークン（スペースなし）
Option<Result<int, string>> nested;  // ❌ パースエラー（スペース必須）
```

**回避策**: 個別変数を使用、または`> >`とスペースを入れる

---

### 2. Async関連

#### ✅ 動作する
```cb
// 基本的なasync関数
async int compute() {
    return 42;
}

// async + Result
async Result<int, string> process() {
    return Result<int, string>::Ok(42);
}

// ?オペレーターとの組み合わせ
async Result<int, string> chain() {
    int val = await process()?;
    return Result<int, string>::Ok(val * 2);
}

// timeout
Future<int> f = timeout(slow_task(), 500);
int result = await f;

// impl内のasyncメソッド
impl Processor {
    async int compute() {
        return 42;
    }
}
```

#### ❌ 未実装
```cb
// async関数を引数に取る
void executor(async int() callback) {  // ❌ パースエラー
    int result = await callback();
}

// asyncラムダ式
auto lambda = async int() {  // ❌ パースエラー
    return 42;
};

// 構造体定義内のasyncメソッド（ジェネリック）
struct Data<T> {
    async T get() {  // ❌ パースエラー
        return this->value;
    }
}
```

**回避策**: impl構文を使用、または関数を直接定義

---

### 3. Match関連

#### ✅ 動作する
```cb
// 1段階のmatch
match (some_val) {
    Some(v) => { println("Value: {v}"); }
    None => { println("None"); }
}

// Result型とのmatch
match (result) {
    Ok(v) => { println("Ok: {v}"); }
    Err(e) => { println("Error: {e}"); }
}
```

#### ❌ 未実装
```cb
// ネストしたmatch（2段階以上）
match (outer) {
    Some(inner) => {
        match (inner) {  // ❌ 型情報が失われる
            Ok(v) => { println("Value: {v}"); }
            Err(e) => { println("Error: {e}"); }
        }
    }
    None => { println("None"); }
}
```

**回避策**: match内で変数に代入してから、別のmatch文を使用

---

### 4. 標準ライブラリ

#### ✅ 動作する
```cb
// Vector<int>
Vector<int> vec;
vec.push_back(10);
int val = vec.at(0);

// Vector<struct>
Vector<Point> vec2;
Point p;
p.x = 10;
vec2.push_back(p);

// TestFramework
TestResult test;
test.assert_eq_int(10, 10, "Test");
```

#### ❌ 既知の問題
```cb
// Vector<string> - セグフォ
Vector<string> vec;
vec.push_back("Hello");  // またはat()でクラッシュ
```

**回避策**: Vector<int>やVector<struct>を使用

---

## 🚧 v0.13.4 計画中の機能

### 優先度: 高
- [ ] 文字列配列の初期化問題の修正
- [ ] Vector<string>のサポート（deep copy実装）
- [ ] ジェネリック構造体配列の完全サポート
- [ ] ネストしたmatch式の型情報保持

### 優先度: 中
- [ ] パフォーマンス最適化
- [ ] エラーメッセージの改善
- [ ] デバッグ機能の強化

---

## 🔮 v0.13.4以降の機能

### v0.13.4: Bug Fixes & Improvements
- [ ] 文字列配列の完全サポート
- [ ] Vector<string>の実装
- [ ] ジェネリック構造体配列の改善
- [ ] ネストしたmatch式の修正

### v0.13.5以降: Advanced Features
- [ ] Async関数型の高度なサポート
- [ ] パフォーマンス最適化
- [ ] 標準ライブラリの拡張

---

## 📊 テストカバレッジ

### v0.13.3時点
- **総テストケース**: 750+
- **Integration tests**: 50個以上
- **成功率**: 100%（`main`ビルドで確認済み）
- **既知の問題**: 4件（文書化済み）

### v0.13.4目標
- **文字列配列修正**: 完全サポート
- **Vector<string>実装**: deep copyメカニズム
- **Generic配列改善**: 型情報の完全保持
- **Nested match修正**: 型推論の改善

---

## 📚 関連ドキュメント

### 実装済み機能
- `release_notes/v0.13.3.md` - Documentation & Known Issues
- `release_notes/v0.13.2.md` - Generic Array String Fix
- `release_notes/v0.13.1.md` - Async Impl & Self Sync
- `release_notes/v0.12.0.md` - Async/Await基本機能
- `release_notes/v0.12.1.md` - ?オペレーターとTimeout
- `release_notes/v0.11.0.md` - Generics & Pattern Matching

### 計画中の機能
- `docs/todo/v0.13.4/` - v0.13.4実装計画（作成予定）
- `docs/features/v0.13.0_untested_behaviors.md` - 未実装・未確認機能

### 設計ドキュメント
- `docs/spec.md` - 言語仕様
- `docs/BNF.md` - 構文定義
- `docs/architecture.md` - アーキテクチャ

---

## 🎯 使用ガイドライン

### 推奨される使い方

1. **ジェネリクス型のネスト**: 必ず`> >`とスペースを入れる
2. **Future配列の回避**: 個別変数を使用
3. **Match式**: 1段階のみ使用、ネストは避ける
4. **Vector**: int/long/struct型のみ使用、stringは避ける

### パフォーマンス

- async/await: オーバーヘッド最小
- Event loop: 協調的マルチタスキング
- ジェネリクス: 実行時型情報管理

---

**メンテナンス**: このドキュメントは各バージョンリリース時に更新されます
