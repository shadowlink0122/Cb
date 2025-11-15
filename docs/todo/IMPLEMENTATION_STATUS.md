# Cb Language Implementation Status

**最終更新**: 2025年11月16日  
**現在のバージョン**: v0.13.1  
**次期バージョン**: v0.13.2 (計画中)

---

## ✅ 実装完了機能

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

## 🚧 v0.13.2 計画中の機能

### 優先度: 高
- [ ] Async関数型およびasyncラムダ構文のサポート
- [ ] impl/structジェネリックasyncメソッドの型パラメータ伝播を完全対応
- [ ] Integration testカバレッジ100%達成（新規asyncテストをCIに組み込み）

### 優先度: 中
- [ ] 仕様書/BNF/feature docsをv0.13.2仕様へ更新
- [ ] 既知の制限事項の自動テスト化と公開

---

## 🔮 v0.13.2以降の機能

### v0.13.2: Error Handling
- [ ] RuntimeError列挙型
- [ ] try式
- [ ] checked式
- [ ] panic/unwrap

### v0.13.3: Advanced Async & Generics
- [ ] Async関数型サポート
- [ ] Asyncラムダ式
- [ ] 構造体内asyncメソッド（ジェネリック）
- [ ] ジェネリクス構造体配列
- [ ] ネストしたmatch式
- [ ] Vector<string>修正

---

## 📊 テストカバレッジ

### v0.13.1時点
- **総テストケース**: 750+（Async impl向けテスト10件追加）
- **Integration tests**: 50個以上（`test_impl_async_method*.cb`, `test_struct_async_method_basic.cb`を含む）
- **成功率**: 100%（`main`/`main-asan`双方で確認済み）

### v0.13.2目標
- **Integration test coverage**: 100%（全Asyncテストを`make test`へ組み込む）
- **自動化**: ASanビルドを含む回帰テストのCI実行
- **新規テスト**: asyncラムダ/関数型・ジェネリックimplの包括テスト

---

## 📚 関連ドキュメント

### 実装済み機能
- `release_notes/v0.13.1.md` - Async Impl & Self Sync
- `release_notes/v0.12.0.md` - Async/Await基本機能
- `release_notes/v0.12.1.md` - ?オペレーターとTimeout
- `release_notes/v0.11.0.md` - Generics & Pattern Matching

### 計画中の機能
- `docs/features/v0.13.0_untested_behaviors.md` - 未実装・未確認機能
- `docs/todo/v0.13.0_generic_array_support.md` - ジェネリクス配列サポート
- `docs/todo/v0.13.0_implementation_plan.md` - v0.13.0実装計画

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
