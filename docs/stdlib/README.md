# Cb Standard Library Documentation

## 概要

Cb標準ライブラリ（stdlib）は、Cb言語で書かれた再利用可能なコンポーネントのコレクションです。全てのライブラリはジェネリクスをサポートし、型安全で効率的なコードを提供します。

## ライブラリ一覧

### コアライブラリ (`stdlib/std/`)

| ライブラリ | 説明 | ドキュメント |
|-----------|------|------------|
| **Vector** | ジェネリック双方向リンクリスト | [vector.md](./vector.md) |
| **Queue** | ジェネリックFIFOキュー | [queue.md](./queue.md) |
| **Map** | ジェネリックハッシュマップ | [map.md](./map.md) |
| **String** | 拡張文字列ライブラリ | [string.md](./string.md) |
| **Test** | テストフレームワーク | [test.md](./test.md) |

### 組み込み型（Built-in Types）

以下の型はv0.11.0でインタプリタに組み込まれました：

- **Option&lt;T&gt;** - オプショナル値（Some/None）
- **Result&lt;T, E&gt;** - 成功/失敗の結果型（Ok/Err）

これらは`import`不要で直接使用できます。

## インストール

標準ライブラリは`stdlib/`ディレクトリに含まれており、Cbインタプリタが自動的に認識します。追加のインストールは不要です。

## 使用方法

### 基本的なimport

```cb
import stdlib.std.vector;
import stdlib.std.queue;
import stdlib.std.map;
import stdlib.std.string;
import stdlib.std.test;
```

### 選択的import（Selective Import）

特定のシンボルのみをimportできます：

```cb
import stdlib.std.vector { Vector };
import stdlib.std.queue { Queue, QueueOps };
import stdlib.std.test { TestResult, print_test_header };
```

### 使用例

#### Vector

```cb
import stdlib.std.vector;

void main() {
    Vector<int> vec;
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);
    
    println("Length: {vec.get_length()}");  // 3
    println("First: {vec.at(0)}");          // 10
    
    vec.sort();  // 昇順ソート
}
```

#### Queue

```cb
import stdlib.std.queue;

void main() {
    Queue<string> q;
    q.push("first");
    q.push("second");
    q.push("third");
    
    string item = q.pop();
    println(item);  // "first"
}
```

#### Map

```cb
import stdlib.std.map;

void main() {
    Map<string, int> ages;
    ages.insert("Alice", 30);
    ages.insert("Bob", 25);
    
    if (ages.contains("Alice")) {
        int age = ages.get("Alice", 0);
        println("Alice is {age} years old");
    }
}
```

#### String

```cb
import stdlib.std.string;

void main() {
    String s = string_from("hello world");
    
    println("Length: {s.size()}");              // 11
    println("Contains 'world': {s.contains("world")}");  // true
    println("Starts with 'hello': {s.starts_with("hello")}");  // true
    
    String upper = s.to_upper();
    println(upper.get());  // "HELLO WORLD"
}
```

## ディレクトリ構造

```
stdlib/
├── std/                    # 標準ライブラリコア
│   ├── vector.cb          # Vector<T> - 双方向リンクリスト
│   ├── queue.cb           # Queue<T> - FIFOキュー
│   ├── map.cb             # Map<K, V> - ハッシュマップ
│   ├── string.cb          # String - 文字列ライブラリ
│   └── test.cb            # TestResult - テストフレームワーク
├── async/                  # 非同期プログラミング（開発中）
│   └── task_queue.cb
├── allocators/             # メモリアロケータ
│   ├── system_allocator.cb
│   └── bump_allocator.cb
├── common/                 # 共通ユーティリティ
├── math.cb                 # 数学関数
└── stdio.cb                # 標準入出力
```

## パフォーマンス特性

### 時間計算量

| 操作 | Vector | Queue | Map |
|------|--------|-------|-----|
| 挿入 | O(1) | O(1) | O(1) 平均 |
| 削除 | O(n)* | O(1) | O(1) 平均 |
| 検索 | O(n) | - | O(1) 平均 |
| アクセス | O(n) | O(1) front | O(1) 平均 |
| ソート | O(n log n) | - | - |

*末尾/先頭からの削除はO(1)

### 空間計算量

- **Vector**: O(n) - 各要素に2つのポインタ（前後）
- **Queue**: O(n) - 各要素に2つのポインタ（前後）
- **Map**: O(n) - ハッシュテーブル + チェイニング
- **String**: O(n) - 文字列データ + 長さフィールド

## ベストプラクティス

### 1. スコープ管理

全てのstdlib構造体はデストラクタを持ち、スコープ終了時に自動的にメモリを解放します：

```cb
void process_data() {
    Vector<int> vec;  // コンストラクタで初期化
    vec.push_back(42);
    // スコープ終了時、自動的に全メモリ解放
}
```

### 2. 型推論の活用

ジェネリクスは型推論をサポートします：

```cb
Vector<int> vec;
vec.push_back(42);  // int型として推論
```

### 3. エラーハンドリング

組み込みのResult型とOption型を使用：

```cb
Option<int> maybe_value = try_get_value();
if (maybe_value.variant == "Some") {
    int value = maybe_value.value;
    println("Got value: {value}");
} else {
    println("No value");
}
```

### 4. テストの作成

testライブラリを使用して包括的なテストを作成：

```cb
import stdlib.std.test;

void test_my_function(TestResult* t) {
    t.assert_eq_int(my_function(5), 25, "5 squared should be 25");
}

void main() {
    TestResult t;
    test_my_function(&t);
    t.print_summary();
}
```

## バージョニング

標準ライブラリはCb言語のバージョンと連動しています：

- **v0.11.0**: 現在の安定版
  - Vector, Queue, Map, String, Test
  - 組み込みOption/Result型
  - ジェネリクス完全サポート

## 今後の予定

### v0.12.0 (予定)
- **Async/Await**: 非同期プログラミングのサポート
- **Iterator**: イテレータトレイト
- **Set**: ハッシュセット実装
- **String改善**: UTF-8完全サポート

### v0.13.0 (予定)
- **File I/O**: ファイル操作ライブラリ
- **Network**: ネットワークライブラリ
- **JSON**: JSONパーサー/シリアライザ

## コントリビューション

標準ライブラリへの貢献を歓迎します！以下のガイドラインに従ってください：

1. **コーディングスタイル**: [CODING_GUIDELINES.md](../CODING_GUIDELINES.md)に従う
2. **テストカバレッジ**: 新機能には包括的なテストを追加
3. **ドキュメント**: 全ての公開APIにドキュメントを追加
4. **パフォーマンス**: 時間/空間計算量を明記

## サポート

- **Issue Tracker**: [GitHub Issues](https://github.com/shadowlink0122/Cb/issues)
- **ドキュメント**: [docs/](../)
- **Examples**: [sample/](../../sample/)

## ライセンス

Cb標準ライブラリはCb言語と同じライセンスの下で提供されます。
