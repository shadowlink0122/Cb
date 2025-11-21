# Cb言語におけるメモリ管理の実装

## 概要
Cb言語のインタプリタとコンパイラでは、C++の標準ライブラリを使用してメモリ管理を実装しています。

## インタプリタでの実装

### 1. malloc関数の実装
**場所**: `src/backend/interpreter/evaluator/functions/call_impl.cpp`

```cpp
// malloc(size) - メモリ確保
if (node->name == "malloc") {
    if (node->arguments.size() != 1) {
        throw std::runtime_error(
            "malloc() requires 1 argument: malloc(size)");
    }

    // 引数からサイズを評価
    int64_t size = interpreter_.eval_expression(node->arguments[0].get());

    if (size <= 0) {
        std::cerr << "[malloc] Error: invalid size " << size << std::endl;
        return 0;
    }

    // C++のstd::mallocを直接呼び出し
    void *ptr = std::malloc(static_cast<size_t>(size));
    if (ptr == nullptr) {
        std::cerr << "[malloc] Error: allocation failed for size "
                  << size << std::endl;
        return 0;
    }

    // デバッグモードの場合、割り当て情報を出力
    if (interpreter_.is_debug_mode()) {
        char dbg_buf[512];
        snprintf(dbg_buf, sizeof(dbg_buf),
                 "Allocated %" PRId64 " bytes at %p", size, ptr);
        debug_msg(DebugMsgId::CALL_IMPL_MALLOC, dbg_buf);
    }

    // ポインタをint64_tとして返す
    return reinterpret_cast<int64_t>(ptr);
}
```

#### 動作の詳細
1. **引数の検証**: `malloc(size)`は1つの引数（サイズ）を受け取る
2. **サイズの評価**: 引数式を評価して実際のバイト数を取得
3. **メモリ確保**: `std::malloc()`でヒープメモリを確保
4. **エラーハンドリング**: 失敗時は`nullptr`（0）を返す
5. **デバッグ出力**: デバッグモードで割り当て情報を記録
6. **ポインタ返却**: void*を`int64_t`にキャストして返す

### 2. new演算子の実装
**場所**: `src/backend/interpreter/evaluator/operators/memory_operators.cpp`

```cpp
int64_t Interpreter::evaluate_new_expression(const ASTNode *node) {
    if (node->is_array_new) {
        // new T[size] - 配列の場合
        int64_t array_size = expression_evaluator_->evaluate_expression(
            node->new_array_size.get());
        size_t element_size = get_type_size(node->new_type_name, this);
        size_t total_size = static_cast<size_t>(array_size) * element_size;

        void *ptr = std::malloc(total_size);
        if (!ptr) {
            throw std::runtime_error("Memory allocation failed");
        }

        // メモリをゼロクリア
        std::memset(ptr, 0, total_size);

        if (debug_mode) {
            std::cerr << "[new] Allocated array: type=" << node->new_type_name
                      << ", size=" << array_size
                      << ", total_bytes=" << total_size << ", ptr=" << ptr
                      << std::endl;
        }

        return reinterpret_cast<int64_t>(ptr);
    } else {
        // new T - 単一オブジェクトの場合
        const StructDefinition *struct_def =
            get_struct_definition(node->new_type_name);

        if (struct_def) {
            // 構造体: Variableオブジェクトをヒープに作成
            Variable *struct_var = new Variable();
            struct_var->type = TYPE_STRUCT;
            struct_var->struct_type_name = node->new_type_name;
            struct_var->is_assigned = true;
            struct_var->is_struct = true;

            // 各メンバーを初期化
            for (const auto &member : struct_def->members) {
                Variable member_var;
                member_var.type = member.type;
                member_var.is_pointer = member.is_pointer;
                member_var.is_assigned = false;
                member_var.value = 0;
                member_var.float_value = 0.0;

                if (member.type == TYPE_STRUCT && !member.type_alias.empty()) {
                    member_var.struct_type_name = member.type_alias;
                }

                struct_var->struct_members[member.name] = member_var;
            }

            return reinterpret_cast<int64_t>(struct_var);
        } else {
            // プリミティブ型: 生メモリを確保
            size_t type_size = get_type_size(node->new_type_name, this);
            void *ptr = std::malloc(type_size);
            if (!ptr) {
                throw std::runtime_error("Memory allocation failed");
            }

            std::memset(ptr, 0, type_size);
            return reinterpret_cast<int64_t>(ptr);
        }
    }
}
```

### 3. free関数の実装
**場所**: `src/backend/interpreter/evaluator/functions/call_impl.cpp`

```cpp
// free(ptr) - メモリ解放
if (node->name == "free") {
    if (node->arguments.size() != 1) {
        throw std::runtime_error(
            "free() requires 1 argument: free(ptr)");
    }

    int64_t ptr_value =
        interpreter_.eval_expression(node->arguments[0].get());

    if (ptr_value == 0) {
        // nullptr の解放は何もしない（安全）
        return 0;
    }

    void *ptr = reinterpret_cast<void *>(ptr_value);
    std::free(ptr);

    if (interpreter_.is_debug_mode()) {
        char dbg_buf[512];
        snprintf(dbg_buf, sizeof(dbg_buf),
                 "Freed memory at %p", ptr);
        debug_msg(DebugMsgId::CALL_IMPL_FREE, dbg_buf);
    }

    return 0;
}
```

## コンパイラ（HIR→C++）での実装

### malloc/freeの生成
**場所**: `src/backend/codegen/hir_to_cpp.cpp`

コンパイラは、CbコードをC++コードに変換する際、`malloc`と`free`をそのままC++の関数として出力します：

```cpp
// Cb言語
let ptr = malloc(100);
free(ptr);

// 生成されるC++コード
auto ptr = malloc(100);
free(ptr);
```

これにより、生成されたC++コードは標準Cライブラリの`malloc`/`free`を直接使用します。

### new/delete演算子の処理
HIRには`HIRExpr::ExprKind::New`が定義されており、`generate_new()`関数で処理されます：

```cpp
std::string HIRToCpp::generate_new(const HIRExpr &expr) {
    // new T[size]またはnew Tを生成
    // 実装は型に応じてmallocまたはC++のnewを使用
}
```

## メモリ管理の特徴

### インタプリタモード
1. **統一されたポインタ表現**: すべてのポインタを`int64_t`として扱う
2. **動的型チェック**: 実行時に型を確認
3. **デバッグサポート**: メモリ割り当て・解放を詳細にログ出力
4. **構造体の特別扱い**: 構造体は`Variable`オブジェクトとして管理
5. **手動メモリ管理**: GCなし、明示的な`free`が必要

### コンパイラモード
1. **C++標準への委譲**: 生成されたC++コードはC++の標準メモリ管理を使用
2. **型安全性**: C++のコンパイラが型チェックを実施
3. **最適化可能**: C++コンパイラによる最適化が適用される
4. **スマートポインタ対応**: 将来的にstd::unique_ptrなどへの変換も可能

## メモリリークの防止

### 現在の実装での注意点
- **手動管理**: Cbは現在GCを持たないため、`malloc`したメモリは必ず`free`する必要がある
- **所有権**: ポインタの所有権を明確にする必要がある
- **二重解放**: 同じポインタを2回`free`しないよう注意が必要

### デバッグモードの活用
```bash
# デバッグモードで実行してメモリ操作をトレース
./cb --debug test.cb
```

デバッグモードでは以下の情報が出力されます：
- メモリ確保時のアドレスとサイズ
- メモリ解放時のアドレス
- 構造体の初期化情報
- ポインタ操作の詳細

## ポインタ実装の内部構造

### ポインタの表現
```cpp
// インタプリタ内部
int64_t ptr_value = reinterpret_cast<int64_t>(actual_pointer);

// 使用時
void* actual_pointer = reinterpret_cast<void*>(ptr_value);
```

### ポインタ演算
```cpp
// ptr + offset の実装例
int64_t result = ptr_value + (offset * element_size);
```

### デリファレンス（*ptr）
```cpp
void* ptr = reinterpret_cast<void*>(ptr_value);
int value = *static_cast<int*>(ptr);  // 型に応じてキャスト
```

## まとめ

Cb言語のメモリ管理は以下のように実装されています：

1. **インタプリタ**: C++の`std::malloc`/`std::free`を直接呼び出し、ポインタを`int64_t`で管理
2. **コンパイラ**: C++コードを生成し、C++標準のメモリ管理機能を利用
3. **デバッグ**: 詳細なログ出力で開発を支援
4. **型安全性**: 実行時（インタプリタ）またはコンパイル時（コンパイラ）に型チェック

この設計により、低レベルのメモリ操作を実現しつつ、デバッグしやすい環境を提供しています。
