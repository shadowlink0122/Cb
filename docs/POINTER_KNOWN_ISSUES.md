# 想定されるバグと理論上の問題点

## 1. メモリ安全性の問題

### ダングリングポインタ
```cb
int* dangling_pointer() {
    int local = 42;
    return &local;  // ローカル変数のアドレスを返す（危険）
}
// 解決策: スコープ検証の実装が必要
```

### Use-After-Free（将来的な問題）
```cb
int* ptr = new int;
*ptr = 42;
delete ptr;
int value = *ptr;  // 解放済みメモリへのアクセス
```

## 2. 型安全性の問題

### ポインタ型の不一致
```cb
float f = 3.14;
int* ip = &f;  // 型が合わない（現在はreinterpret_castで強制変換）
*ip = 42;  // float変数にint値を書き込む
```

### 配列境界外アクセス
```cb
int[5] arr = {1, 2, 3, 4, 5};
int* ptr = &arr[0];
ptr = ptr + 10;  // 配列の範囲外
*ptr = 999;  // 未定義動作
```

## 3. 現在の実装の具体的な問題

### 配列要素へのポインタの誤動作
```cb
int[3] arr = {10, 20, 30};
int* ptr = &arr[0];
*ptr = 100;  // 期待: arr[0] = 100
// 実際: メモリ破壊の可能性
```
**原因**: `reinterpret_cast<int64_t*>`を`Variable*`としてキャスト

### 構造体メンバーへのポインタの誤動作
```cb
struct Point { int x; int y; }
Point p = {x: 1, y: 2};
int* px = &p.x;
*px = 10;  // 期待: p.x = 10
// 実際: 動作するが、偶然うまくいっているだけ
```

### ポインタ演算の未実装
```cb
int* ptr = &arr[0];
ptr++;  // 期待: 次の要素を指す
        // 実際: ポインタのアドレス値が1増えるだけ（型サイズを考慮していない）
```

## 4. float/double配列の初期化バグ

### 配列リテラルが設定されない
```cb
int[3] arr = {10, 20, 30};  // 実際: {0, 0, 0}
float[3] farr = {1.5, 2.5, 3.5};  // 実際: {0.0, 0.0, 0.0}
```
**影響範囲**: グローバル配列、ローカル配列の両方

## 5. ポインタ比較の問題

### 異なる型のポインタ比較
```cb
int x = 10;
float f = 3.14;
int* px = &x;
float* pf = &f;
if (px == pf) { }  // 型が違うポインタの比較
```

### nullptr以外の特殊値
```cb
int* ptr = 0;  // nullptr
int* ptr2 = -1;  // 無効なポインタだが検出されない
```

## 6. マルチスレッド環境（将来的な問題）

### データ競合
```cb
// スレッド1
*shared_ptr = 10;

// スレッド2  
int value = *shared_ptr;
```

## 7. ポインタのアライメント問題

### 未アライメントアクセス
```cb
char[16] buffer;
int* misaligned = (int*)(&buffer[1]);  // 奇数アドレス
*misaligned = 42;  // 一部のアーキテクチャでクラッシュ
```

## 8. implブロック内のself問題

### selfのポインタメンバーアクセス
```cb
struct Node {
    int* next_ptr;
}

impl SomeInterface for Node {
    void process() {
        *self.next_ptr = 42;  // selfの解決が正しくない可能性
    }
}
```

## 9. 関数ポインタ（未実装）

```cb
int (*func_ptr)(int, int);
func_ptr = &add;  // 関数のアドレスを取得
int result = func_ptr(1, 2);  // 関数ポインタ経由で呼び出し
```

## 10. const correctness（未実装）

```cb
const int x = 10;
int* ptr = &x;  // constを無視
*ptr = 20;  // const変数を変更してしまう
```

## 優先度の整理

### 🔴 Critical（すぐに修正すべき）
1. 配列初期化バグ
2. 配列要素へのポインタの間接参照

### 🟡 High（メタデータ導入で解決）
3. ポインタ演算
4. 構造体メンバーへのポインタ

### 🟢 Medium（将来的に対応）
5. 型安全性チェック
6. スコープ検証
7. 境界チェック

### ⚪ Low（現時点では対応不要）
8. マルチスレッド
9. 関数ポインタ
10. const correctness
