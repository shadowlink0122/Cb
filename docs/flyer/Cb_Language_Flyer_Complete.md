# Cb (シーフラット) プログラミング言語

**v0.10.0 - ムーブセマンティクス完全実装版**

---

## 🎯 なぜCbを作ったのか？

### 背景と動機

> **「使いやすく、安全で、高速な言語」を作りたかった**

プログラミング言語の設計には常にトレードオフが存在します。C++の強力さ、Rustの安全性、TypeScriptの使いやすさ——これらを統合した言語を目指しました。

#### 既存言語の課題

- **C/C++**: 強力だが型安全性が弱く、メモリ管理が複雑
- **TypeScript**: 優れた型システムだがランタイムオーバーヘッドが大
- **Rust**: 最高の安全性だが学習曲線が非常に急峻

#### Cbが目指す3つの柱

1. **実用性重視** - 実際に使える機能を段階的に追加。理論より実践を優先
2. **型安全性** - コンパイル時の厳密な型チェックで実行時エラーを防ぐ
3. **学習しやすさ** - C/C++経験者がすぐに書ける親しみやすい構文

#### 開発者の視点

個人開発だからこそ、実験的な機能を迅速に試し、フィードバックを即座に反映できます。大規模言語にはない柔軟性と進化の速さが強みです。

### 開発の経緯

- **v0.6.0** (2024): 基本型、配列、構造体
- **v0.8.0** (2024): Union型、Interface/Impl
- **v0.9.0**: 関数ポインタ、参照型
- **v0.10.0**: ムーブセマンティクス

### 開発の原動力

- コンパイラ理論への興味と実践
- 2,954個のテストで品質保証
- GitHubでのオープンソース開発

### 設計原則

- **ゼロコスト抽象化** - 最小オーバーヘッド
- **明示的メモリ管理** - GCなし、RAII
- **段階的学習** - 基本→高度な機能
- **実用性重視** - 使いやすさを優先

### 技術スタック

| 項目 | 内容 |
|------|------|
| **言語** | C++17 |
| **実行** | ASTインタープリタ |
| **ビルド** | Make (macOS/Linux) |

---

## 🔥 Cbの特徴

### 1. ムーブセマンティクス

**ゼロコストの所有権移動**

```cb
struct Resource {
    int* data;
    int size;
}

impl Resource {
    // ムーブコンストラクタ
    self(Resource&& other) {
        self.data = other.data;
        self.size = other.size;
        other.data = nullptr;  // 元を無効化
        other.size = 0;
    }
    
    ~self() {
        if (self.data != nullptr) {
            // リソース解放
            free(self.data);
        }
    }
}

void main() {
    Resource r1(100);
    Resource r2 = move(r1);  // 所有権移動
}
```

### 2. TypeScript風Union型

```cb
typedef Status = 200 | 404 | 500;
typedef Value = int | string;

Status code = 200;
Value data = 42;
data = "Hello";  // 型変更OK
```

### 3. Rust風Interface/Impl

```cb
interface Drawable {
    void draw();
}

struct Circle {
    int radius;
}

impl Drawable for Circle {
    void draw() {
        println("Circle", self.radius);
    }
}
```

### 4. 関数ポインタ

```cb
int add(int a, int b) {
    return a + b;
}
void main() {
    int* op = &add;
    println(op(5, 3));  // 8
}
```

---

## 🌟 他の言語との比較

| 特徴 | **Cb** | C++ | TS | Rust |
|------|--------|-----|-----|------|
| 型安全性 | ✅ 強 | △ 弱 | ✅ 強 | ✅ 最強 |
| メモリ管理 | ✅ RAII | ✅ RAII | ❌ GC | ✅ 所有権 |
| ムーブ | ✅ | ✅ | ❌ | ✅ |
| 学習コスト | 🟢 低 | 🔴 高 | 🟢 低 | 🔴 高 |
| 実行速度 | 🟡 中 | 🟢 高 | 🔴 遅 | 🟢 高 |
| Union型 | ✅ | ❌ | ✅ | ✅ |
| Interface | ✅ | ❌ | ✅ | ✅ |

### Cbが優れている点

- **低い学習コスト** - C/C++に似た構文
- **型安全性** - Union型、Interface/Impl
- **実用的** - 関数ポインタ、ムーブセマンティクス
- **開発が容易** - シンプルなビルド、明確なエラー

---

## 🎯 将来のビジョン

### Phase 1: 基礎完成 ✅ (v0.1-v0.10.0)

- ✅ 基本型、配列、構造体、ポインタ
- ✅ コンストラクタ/デストラクタ
- ✅ ムーブセマンティクス、完全なRAII

### Phase 2: 高度な機能 🚧 (v0.11-v1.0)

- 🔲 ジェネリクス/テンプレート
- 🔲 スマートポインタ（unique_ptr, shared_ptr）
- 🔲 Result型エラーハンドリング
- 🔲 標準ライブラリ拡充

### Phase 3: 最適化 🎯 (v1.1+)

- 🎯 LLVMバックエンド（ネイティブコンパイル）
- 🎯 VSCode拡張、パッケージマネージャー
- 🎯 静的解析、デバッガー統合

---

## 📊 品質指標

| 項目 | 値 |
|------|-----|
| **統合テスト** | 2,924個（100%成功）🎉 |
| **ユニットテスト** | 30個（100%成功）🎉 |
| **総テスト数** | 2,954個（100%成功）🎉 |
| **コード行数** | 約50,000行 |

---

## 📝 サンプルプログラム

### 例1: FizzBuzz（Interface活用）

```cb
interface IFizzbuzz { void fizzbuzz(); }

impl IFizzbuzz for int {
    void fizzbuzz() {
        if (self % 15 == 0) println("FizzBuzz");
        else if (self % 3 == 0) println("Fizz");
        else if (self % 5 == 0) println("Buzz");
        else println(self);
    }
}
int main() {
    for (int i = 1; i <= 100; i++) {
        i.fizzbuzz();  // メソッド呼び出し！
    }
}
```

**✨ ポイント:** プリミティブ型（int）にメソッド追加、selfで現在の値を参照

### 例2: リソース管理（RAII）

```cb
struct File { string name; bool is_open; }

impl File {
    self(string fn) {
        self.name = fn; self.is_open = true;
        println("File opened:", fn);
    }
    ~self() {
        if (self.is_open)
            println("File closed:", self.name);
    }
}
void main() {
    { File f("data.txt"); }  // 自動でデストラクタ実行
    println("Outside");
}
```

**出力:**
```
File opened: data.txt
File closed: data.txt
Outside
```

---

## 🚀 クイックスタート

```bash
git clone https://github.com/shadowlink0122/Cb.git
cd Cb && make
echo 'void main() { println("Hello!"); }' > hello.cb
./main hello.cb
./main sample/fizzbuzz.cb  # サンプル
```

---

## 📚 詳細情報

- **GitHub**: [github.com/shadowlink0122/Cb](https://github.com/shadowlink0122/Cb)
- **ドキュメント**: `docs/spec.md`, `docs/tutorial/basic_syntax_guide.md`
- **リリースノート**: `release_notes/v0.10.0.md`

---

## 📄 著作情報

**Cb言語 v0.10.0**  
C++の表現力とTypeScriptの型安全性を融合  
開発者: shadowlink0122 | 更新: 2025/10/12

---

> **"使いやすく、安全で、高速な言語"**
