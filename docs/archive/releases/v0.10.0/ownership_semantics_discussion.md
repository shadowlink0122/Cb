# 所有権セマンティクス 議論ドキュメント

## 📋 概要
v0.10.0デストラクタ実装完了後、所有権セマンティクスについて明確化が必要。

## 🤔 議論すべき質問

### 1. 参照（`T&`）は所有権を渡すか？

#### 現在の実装状況

**デストラクタの挙動**（`interpreter.cpp:2180`）:
```cpp
// 値メンバー（ポインタでも参照でもない）で構造体型の場合
if (member.type == TYPE_STRUCT && !member.is_pointer &&
    !member.is_reference && !member.type_alias.empty()) {
    // デストラクタを再帰的に登録
    register_destructor_call(member_var_name, member_type);
}
```

**現在の動作**:
- ✅ **値メンバー**: デストラクタを呼ぶ（所有権あり）
- ❌ **ポインタメンバー**: デストラクタを呼ばない（所有権なし）
- ❌ **参照メンバー**: デストラクタを呼ばない（所有権なし）

#### 選択肢と考察

**選択肢A: 参照は所有権を渡さない（現在の実装）** ✅ 推奨

**理由**:
1. **C++のセマンティクスとの整合性**
   - C++では参照は「エイリアス」であり、所有権を持たない
   - 参照先のオブジェクトのライフタイムは参照の責任外

2. **予測可能な動作**
   ```cb
   struct Inner { ~self() { println("Inner destroyed"); } }
   struct Outer { Inner& ref; ~self() { println("Outer destroyed"); } }
   
   void main() {
       Inner inner;
       Outer outer;
       outer.ref = &inner;
   }  // 期待: Outer destroyed → Inner destroyed（1回だけ）
   ```

3. **ダブルフリーの防止**
   - 参照メンバーがデストラクタを呼ぶと、同じオブジェクトが2回破壊される危険性

**選択肢B: 参照は所有権を渡す** ❌ 非推奨

**問題点**:
- ダブルフリーのリスク
- C++やRustとの一貫性がない
- 所有権の追跡が複雑になる

**決定**: **選択肢A（参照は所有権なし）を採用**

---

### 2. ムーブコンストラクタは所有権を渡すべきか？

#### 現在の実装状況

**計画されているシグネチャ**:
```cb
// コピーコンストラクタ（const参照）
self(const Point& other) {
    self.x = other.x;
    self.y = other.y;
    println("Copy constructor called");
}

// ムーブコンストラクタ（右辺値参照 - 計画中）
self(Point&& other) {
    self.x = other.x;
    self.y = other.y;
    // 元のオブジェクトを無効化
    other.x = 0;
    other.y = 0;
    println("Move constructor called");
}
```

#### 選択肢と考察

**選択肢A: ムーブコンストラクタは所有権を完全に移動する** ✅ 推奨

**理由**:
1. **リソース管理の明確化**
   ```cb
   struct Buffer {
       int* data;
       int size;
   }
   
   impl Buffer {
       // ムーブコンストラクタ
       self(Buffer&& other) {
           self.data = other.data;
           self.size = other.size;
           
           // 所有権移動: 元のオブジェクトを無効化
           other.data = nullptr;
           other.size = 0;
       }
       
       ~self() {
           if (self.data != nullptr) {
               // メモリ解放（仮想的）
               println("Buffer destroyed");
           }
       }
   }
   
   void main() {
       Buffer b1(100);
       Buffer b2 = move(b1);  // 所有権移動
       // b1.dataはnullptr、デストラクタは何もしない
   }  // b2のデストラクタだけがメモリ解放
   ```

2. **パフォーマンス最適化**
   - ディープコピーを避ける
   - ポインタのコピーのみ（O(1)）

3. **C++/Rustとの整合性**
   - C++のムーブセマンティクスと同じ
   - Rustの`std::mem::swap`や所有権移動と同じ概念

**選択肢B: ムーブでもコピーを行う（浅いコピー）** ❌ 非推奨

**問題点**:
- ダブルフリーのリスク
- ムーブの意味がない（コピーと同じ）
- パフォーマンス最適化の利点がない

**決定**: **選択肢A（ムーブは所有権を完全に移動）を採用**

---

## 📊 所有権マトリックス

| 型 | デストラクタ呼び出し | 所有権 | 用途 |
|---|---|---|---|
| `T` 値メンバー | ✅ 呼ぶ | あり | 組み込みオブジェクト |
| `T*` ポインタ | ❌ 呼ばない | なし | 外部オブジェクトへの参照 |
| `T&` 参照 | ❌ 呼ばない | なし | エイリアス（別名） |
| `T&&` ムーブ元 | ❌ 呼ばない（無効化） | なし（移動済み） | ムーブ後のオブジェクト |
| `T&&` ムーブ先 | ✅ 呼ぶ | あり（受け取り） | 所有権を受け取った |

---

## 🎯 推奨される設計

### 値メンバー（所有権あり）
```cb
struct Outer {
    Inner member;  // 値メンバー = 所有権あり
}

impl Outer {
    ~self() {
        // member.~self()が自動的に呼ばれる（LIFO順序）
    }
}
```

### ポインタメンバー（所有権なし）
```cb
struct Outer {
    Inner* ptr;  // ポインタ = 所有権なし（外部管理）
}

impl Outer {
    ~self() {
        // ptr->~self()は呼ばれない
        // 必要に応じて手動で delete ptr; など
    }
}
```

### 参照メンバー（所有権なし）
```cb
struct Outer {
    Inner& ref;  // 参照 = エイリアス、所有権なし
}

impl Outer {
    ~self() {
        // ref.~self()は呼ばれない
        // 参照先のオブジェクトは別の場所で管理される
    }
}
```

### ムーブコンストラクタ（所有権移動）
```cb
impl Buffer {
    self(Buffer&& other) {
        // 所有権を受け取る
        self.data = other.data;
        self.size = other.size;
        
        // ムーブ元を無効化（重要！）
        other.data = nullptr;
        other.size = 0;
    }
    
    ~self() {
        // ムーブ元: data==nullptr → 何もしない
        // ムーブ先: data!=nullptr → 解放
        if (self.data != nullptr) {
            // メモリ解放
        }
    }
}
```

---

## 🔍 実装への影響

### 1. デストラクタ（v0.10.0 - 完了）
✅ **現在の実装は正しい**:
- 値メンバー: デストラクタ呼び出し ✅
- ポインタ/参照: デストラクタ呼び出しスキップ ✅

### 2. ムーブコンストラクタ（v0.11.0 - 計画中）
**必要な実装**:
1. `&&` 右辺値参照の構文サポート
2. `move()` 関数の実装
3. ムーブコンストラクタの検出
4. ムーブ後のオブジェクト無効化

**例**: `tests/cases/constructor/move_constructor_test.cb`
```cb
Buffer b1(100);
Buffer b2 = move(b1);  // ムーブコンストラクタ呼び出し
// b1は無効化されている（data==nullptr）
// b2が所有権を持つ
```

### 3. コピーコンストラクタ（v0.10.0 - 部分実装）
**現在**:
- `self(const T& other)` のシグネチャ検出 ✅
- メンバーワイズコピー ✅

**将来**:
- ディープコピーのサポート
- ネストした構造体のコピー最適化

---

## 📝 ドキュメント更新

### 更新すべきファイル

1. **`docs/spec.md`**
   - 所有権セマンティクスのセクションを追加
   - 参照、ポインタ、ムーブの違いを明確化

2. **`docs/features/constructor_destructor.md`**
   - 所有権マトリックスを追加
   - ムーブコンストラクタの実装詳細

3. **`release_notes/v0.10.0.md`**
   - デストラクタの所有権ルールを記載

---

## 💡 結論

### 決定事項（2025-10-11 更新）

1. **参照（`T&`）は所有権を渡さない** ✅
   - 理由: エイリアス、ダブルフリー防止、C++との整合性
   - 実装: 既に正しく実装済み

2. **右辺値参照（`T&&`）で完全に所有権を移動する** ✅ **確定**
   - 理由: リソース管理、パフォーマンス、C++との完全な整合性
   - 実装: v0.11.0で実装開始

3. **通常の変数でもムーブ（所有権の移動）を実装** ✅ **確定**
   - `move()` 関数を使用して明示的にムーブ
   - ムーブ後、ソース変数はデストラクタ呼び出しなし

4. **右辺値のムーブは禁止** ✅ **確定**
   - リテラル、一時オブジェクトは `move()` できない
   - 左辺値（変数）のみムーブ可能

### 次のステップ

- [x] ドキュメント更新完了
- [x] v0.11.0実装計画作成完了（`docs/todo/v0.11.0_move_semantics_implementation.md`）
- [ ] 実装開始（Phase 1: データ構造の拡張）
- [ ] テストケース作成（所有権移動の検証）

---

## 📚 参考資料

### C++の所有権セマンティクス
```cpp
// C++の例
struct Buffer {
    int* data;
    
    // コピー: 所有権は共有されない（ディープコピー）
    Buffer(const Buffer& other) {
        data = new int[other.size];
        std::copy(other.data, other.data + size, data);
    }
    
    // ムーブ: 所有権を移動
    Buffer(Buffer&& other) noexcept {
        data = other.data;
        other.data = nullptr;  // 無効化
    }
    
    ~Buffer() {
        delete[] data;  // nullptr のときは何もしない
    }
};
```

### Rustの所有権
```rust
// Rustの例
struct Buffer {
    data: Vec<i32>,
}

fn main() {
    let b1 = Buffer { data: vec![1, 2, 3] };
    let b2 = b1;  // ムーブ（コピーではない）
    // b1 はもう使えない（コンパイルエラー）
}
```

### Cb言語の目標
```cb
// Cb言語の設計
struct Buffer {
    int* data;
    int size;
}

impl Buffer {
    // コピー: ディープコピー（明示的）
    self(const Buffer& other) {
        // 実装...
    }
    
    // ムーブ: 所有権移動（軽量）
    self(Buffer&& other) {
        self.data = other.data;
        other.data = nullptr;  // 無効化
    }
}

void main() {
    Buffer b1(100);
    Buffer b2 = b1;        // コピー（重い）
    Buffer b3 = move(b1);  // ムーブ（軽い）
}
```

**Cb言語の強み**: C++のシンプルさ + Rustの安全性の良いとこ取り
