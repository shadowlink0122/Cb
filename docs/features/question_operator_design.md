# ?オペレーター設計仕様

**バージョン**: v0.12.1  
**ステータス**: 設計中

---

## 概要

Rustスタイルの`?`オペレーターを実装し、`Result<T, E>`と`Option<T>`のエラー伝播を簡潔に記述できるようにします。

---

## 構文

```cb
expression?
```

---

## 動作

### Result<T, E>の場合

```cb
Result<int, string> divide(int a, int b) {
    if b == 0 {
        return Result::Err("Division by zero");
    }
    return Result::Ok(a / b);
}

Result<int, string> complex_calculation(int x, int y) {
    int result1 = divide(x, y)?;  // Errの場合は即座にreturn
    int result2 = divide(result1, 2)?;
    return Result::Ok(result2);
}
```

**展開後のコード**:
```cb
Result<int, string> complex_calculation(int x, int y) {
    Result<int, string> _tmp1 = divide(x, y);
    if _tmp1.is_err() {
        return _tmp1;  // エラーをそのまま伝播
    }
    int result1 = _tmp1.unwrap();
    
    Result<int, string> _tmp2 = divide(result1, 2);
    if _tmp2.is_err() {
        return _tmp2;
    }
    int result2 = _tmp2.unwrap();
    
    return Result::Ok(result2);
}
```

### Option<T>の場合

```cb
Option<int> find(int[] arr, int target) {
    for int i = 0; i < arr.len; i++ {
        if arr[i] == target {
            return Option::Some(i);
        }
    }
    return Option::None;
}

Option<int> find_and_double(int[] arr, int target) {
    int index = find(arr, target)?;  // Noneの場合は即座にreturn
    return Option::Some(arr[index] * 2);
}
```

**展開後のコード**:
```cb
Option<int> find_and_double(int[] arr, int target) {
    Option<int> _tmp = find(arr, target);
    if _tmp.is_none() {
        return Option::None;
    }
    int index = _tmp.unwrap();
    return Option::Some(arr[index] * 2);
}
```

---

## 型制約

### 1. 戻り値の型が一致する必要がある

```cb
// ❌ エラー: 関数の戻り値がResult<T, E>ではない
int func() {
    int x = divide(10, 2)?;  // コンパイルエラー
    return x;
}

// ✅ OK: 戻り値の型が一致
Result<int, string> func() {
    int x = divide(10, 2)?;
    return Result::Ok(x);
}
```

### 2. エラー型の互換性

```cb
// ❌ エラー: エラー型が異なる
Result<int, int> func() {
    int x = divide(10, 2)?;  // Result<int, string>のエラーをResult<int, int>で返せない
    return Result::Ok(x);
}

// ✅ OK: エラー型が一致
Result<int, string> func() {
    int x = divide(10, 2)?;
    return Result::Ok(x);
}
```

---

## async関数での使用

```cb
async Result<int, string> async_divide(int a, int b) {
    if b == 0 {
        return Result::Err("Division by zero");
    }
    return Result::Ok(a / b);
}

async Result<int, string> async_calc() {
    int x = await async_divide(10, 2)?;
    int y = await async_divide(x, 3)?;
    return Result::Ok(y);
}
```

---

## 実装方針

### フェーズ1: パーサー拡張

1. `?`を後置単項演算子として認識
2. `PostfixExpression`に`QuestionOperator`ノードを追加
3. 優先順位: 関数呼び出しやメンバーアクセスの後

### フェーズ2: 型チェック

1. `?`が付いた式の型が`Result<T, E>`または`Option<T>`であることを確認
2. 関数の戻り値の型が一致することを確認
3. エラー型の互換性を確認

### フェーズ3: コード生成

1. 一時変数の生成
2. `is_err()`/`is_none()`チェックの挿入
3. early returnの生成
4. `unwrap()`呼び出しの生成

---

## テストケース

### test_question_operator_result.cb

```cb
Result<int, string> divide(int a, int b) {
    if b == 0 {
        return Result::Err("Division by zero");
    }
    return Result::Ok(a / b);
}

Result<int, string> chain_divide(int x) {
    int a = divide(x, 2)?;
    int b = divide(a, 3)?;
    int c = divide(b, 4)?;
    return Result::Ok(c);
}

int main() {
    Result<int, string> r1 = chain_divide(240);
    assert(r1.is_ok());
    assert(r1.unwrap() == 10);  // 240 / 2 / 3 / 4 = 10
    
    Result<int, string> r2 = chain_divide(0);
    assert(r2.is_err());
    
    println("? operator test passed");
    return 0;
}
```

### test_question_operator_option.cb

```cb
Option<int> find(int[] arr, int target) {
    for int i = 0; i < arr.len; i++ {
        if arr[i] == target {
            return Option::Some(i);
        }
    }
    return Option::None;
}

Option<int> find_and_double(int[] arr, int target) {
    int idx = find(arr, target)?;
    return Option::Some(arr[idx] * 2);
}

int main() {
    int[] arr = [1, 2, 3, 4, 5];
    
    Option<int> r1 = find_and_double(arr, 3);
    assert(r1.is_some());
    assert(r1.unwrap() == 6);  // arr[2] * 2 = 3 * 2
    
    Option<int> r2 = find_and_double(arr, 10);
    assert(r2.is_none());
    
    println("? operator option test passed");
    return 0;
}
```

---

## 制限事項（v0.12.1）

1. **型変換なし**: エラー型の自動変換は未サポート
2. **カスタム型**: `Result`と`Option`以外の型には未対応
3. **複雑な式**: `(a? + b?)`のような複雑な式は未サポート

---

## 将来の拡張（v0.13.0以降）

1. **FromError trait**: エラー型の自動変換
2. **カスタム型サポート**: `Try` traitの実装
3. **複雑な式**: 演算子の組み合わせ

