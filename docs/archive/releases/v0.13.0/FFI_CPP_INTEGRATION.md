# FFIで複雑なC++コードから生成された動的ライブラリを読み込む - 完全ガイド

## 質問への回答

**Q: FFIで複雑なC++コードから生成された動的ライブラリを読み込むことはできますか？**

**A: はい、可能です！** Cb言語のFFI (Foreign Function Interface) は、C ABIを公開する任意のC++ライブラリを読み込むことができます。

## 必須条件

### 1. C++側の準備

C++の関数を`extern "C"`でラップする必要があります：

```cpp
// complex_library.cpp
#include <vector>
#include <algorithm>
#include <cmath>

// C++の複雑なクラス（内部実装）
class ComplexCalculator {
public:
    double calculate(double x, double y) {
        std::vector<double> data = {x, y};
        std::sort(data.begin(), data.end());
        return std::sqrt(data[0] * data[0] + data[1] * data[1]);
    }
};

// FFI用にC ABIでエクスポート
extern "C" {
    // グローバルインスタンス（または動的に管理）
    ComplexCalculator* create_calculator() {
        return new ComplexCalculator();
    }
    
    void destroy_calculator(ComplexCalculator* calc) {
        delete calc;
    }
    
    double calculator_process(double x, double y) {
        ComplexCalculator calc;
        return calc.calculate(x, y);
    }
    
    // より複雑な操作
    int process_array(double* input, int size, double* output) {
        std::vector<double> vec(input, input + size);
        std::sort(vec.begin(), vec.end());
        
        // 処理後の結果を出力配列にコピー
        for (int i = 0; i < size; i++) {
            output[i] = vec[i] * 2.0;
        }
        return size;
    }
}
```

### 2. コンパイル

```bash
# macOS (ARM64)
clang++ -shared -fPIC complex_library.cpp -o libcomplex.dylib

# macOS (Universal Binary)
clang++ -arch arm64 -arch x86_64 -shared -fPIC complex_library.cpp -o libcomplex.dylib

# Linux
g++ -shared -fPIC complex_library.cpp -o libcomplex.so

# アーキテクチャの確認
file libcomplex.dylib
# 出力例: Mach-O 64-bit dynamically linked shared library arm64
```

### 3. ライブラリの配置

```bash
# 推奨: stdlib/foreignディレクトリ
mkdir -p stdlib/foreign
cp libcomplex.dylib stdlib/foreign/

# または、カレントディレクトリ
cp libcomplex.dylib .
```

### 4. Cb側での使用

```cb
// complex_example.cb
use foreign.complex {
    double calculator_process(double x, double y);
}

void main() {
    double result = complex.calculator_process(3.0, 4.0);
    println("Result:", result);
}
```

## 実際の動作例

### 動作確認済みの例

プロジェクトには実際に動作するC++ライブラリの例が含まれています：

**ファイル**: `sample/ffi_cpp_example.cpp` と `sample/ffi_cpp_example.cb`

```bash
# コンパイル
cd sample
clang++ -shared -fPIC ffi_cpp_example.cpp -o libcppexample.dylib
cp libcppexample.dylib ../stdlib/foreign/

# 実行
cd ..
./main sample/ffi_cpp_example.cb
```

**出力**:
```
=== FFI C++ Example ===

Basic Arithmetic:
  10 + 20 = 30
  7 * 8 = 56

Math Operations:
  Distance from (0,0) to (3,4) = 5.0
  Area of circle with radius 5 = 78.53981633975

Factorial:
   1 ! = 1
   2 ! = 2
   ...
   10 ! = 3628800

Fibonacci:
  fib(0) = 0
  fib(1) = 1
  ...
  fib(10) = 55

Void Functions:
Hello from C++!
C++ received number: 42

=== All tests completed! ===
```

## 対応している複雑なC++機能

### ✅ 使用可能

1. **STLコンテナ** (内部使用)
   ```cpp
   extern "C" int process_with_vector(double* data, int size) {
       std::vector<double> vec(data, data + size);
       std::sort(vec.begin(), vec.end());
       return vec.size();
   }
   ```

2. **クラスとオブジェクト** (内部使用)
   ```cpp
   class MyClass {
       void internal_method();
   };
   
   extern "C" void public_function() {
       MyClass obj;
       obj.internal_method();
   }
   ```

3. **テンプレート** (内部使用)
   ```cpp
   template<typename T>
   T max(T a, T b) { return a > b ? a : b; }
   
   extern "C" int get_max_int(int a, int b) {
       return max<int>(a, b);
   }
   ```

4. **例外処理** (内部使用、C境界では慎重に)
   ```cpp
   extern "C" int safe_divide(int a, int b) {
       try {
           if (b == 0) throw std::runtime_error("Division by zero");
           return a / b;
       } catch (...) {
           return -1; // エラーコードを返す
       }
   }
   ```

5. **名前空間** (内部使用)
   ```cpp
   namespace MyNamespace {
       int complex_calc(int x) { return x * x; }
   }
   
   extern "C" int public_calc(int x) {
       return MyNamespace::complex_calc(x);
   }
   ```

### ❌ 直接使用不可（C ABI制約）

1. **C++クラスの直接エクスポート**
   ```cpp
   // ❌ これは動作しない
   class MyClass {
   public:
       void method();
   };
   ```

2. **C++例外のスロー（C境界を超える）**
   ```cpp
   // ❌ 危険
   extern "C" void dangerous() {
       throw std::runtime_error("Error");
   }
   ```

3. **オーバーロード関数**
   ```cpp
   // ❌ C ABIはオーバーロードをサポートしない
   extern "C" {
       int func(int x);
       int func(double x);  // エラー
   }
   ```

## 複雑なC++ライブラリの統合パターン

### パターン1: ラッパークラス

```cpp
// C++の複雑な実装
class ComplexEngine {
    std::map<std::string, std::vector<double>> data;
public:
    void process(const std::string& key, const std::vector<double>& values);
    double get_result(const std::string& key);
};

// C API ラッパー
extern "C" {
    // オペークポインタパターン
    typedef void* EngineHandle;
    
    EngineHandle engine_create() {
        return new ComplexEngine();
    }
    
    void engine_destroy(EngineHandle handle) {
        delete static_cast<ComplexEngine*>(handle);
    }
    
    void engine_process(EngineHandle handle, const char* key, 
                       double* values, int size) {
        auto* engine = static_cast<ComplexEngine*>(handle);
        std::vector<double> vec(values, values + size);
        engine->process(key, vec);
    }
    
    double engine_get_result(EngineHandle handle, const char* key) {
        auto* engine = static_cast<ComplexEngine*>(handle);
        return engine->get_result(key);
    }
}
```

### パターン2: シンプルファサード

```cpp
// 複雑な内部実装
namespace Internal {
    class ComplexAlgorithm {
        // 複雑なロジック
    };
}

// シンプルなC API
extern "C" {
    int run_algorithm(double* input, int input_size,
                     double* output, int* output_size) {
        Internal::ComplexAlgorithm algo;
        // 複雑な処理を隠蔽
        std::vector<double> result = algo.process(input, input_size);
        
        *output_size = result.size();
        std::copy(result.begin(), result.end(), output);
        return 0; // success
    }
}
```

## v0.13.0 の残りの実装状況

### ✅ 完了した機能

1. **FFI (Foreign Function Interface)**
   - ✅ C/C++/Rust/Zig/Go 対応
   - ✅ 動的ライブラリのロード (.so/.dylib/.dll)
   - ✅ 基本的な型のサポート (int, long, double, void)
   - ✅ 複数パラメータのサポート
   - ✅ 実際のC++ライブラリで動作確認済み

2. **VSCode拡張機能**
   - ✅ シンタックスハイライト
   - ✅ コメントトグル
   - ✅ 自動括弧閉じ
   - ✅ VSIXパッケージ作成済み (`vscode-extension/cb-language-0.13.0.vsix`)

3. **プリプロセッサ**
   - ✅ `#define` / `#undef`
   - ✅ `#ifdef` / `#ifndef` / `#else` / `#endif`
   - ✅ `#error` / `#warning`
   - ✅ 組み込みマクロ (`__FILE__`, `__LINE__`, `__DATE__`, `__TIME__`, `__VERSION__`)

4. **C風マクロ**
   - ✅ 定数マクロ
   - ✅ 関数マクロ
   - ✅ 複数行マクロ

### 🚧 拡張可能な領域

1. **FFI拡張**
   - ポインタのより高度なサポート
   - 構造体の受け渡し
   - コールバック関数

2. **テストの追加**
   - より多くのFFIテストケース
   - エッジケースのテスト

3. **ドキュメント**
   - より多くの実例
   - ベストプラクティスガイド

## テスト

### FFIの統合テスト

```bash
# 既存のFFIテストを実行
./main tests/integration/cases/ffi/math_functions.cb

# 出力:
# Test 1: sqrt - PASSED
# Test 2: pow - PASSED
# Test 3: sin(0) - PASSED
# Test 4: cos(0) - PASSED
```

### C++の例をテスト

```bash
./main sample/ffi_cpp_example.cb
# 全てのテストが成功することを確認
```

## まとめ

### FFIで複雑なC++ライブラリは使える？

**はい！** 以下の条件で使用可能：

1. ✅ **extern "C"** でラップする
2. ✅ **C ABI互換**の型を使用
3. ✅ **適切にコンパイル**（正しいアーキテクチャ）
4. ✅ **stdlib/foreign/** に配置

### v0.13.0の実装状況

| 機能 | ステータス |
|-----|----------|
| FFI基本機能 | ✅ 完了 |
| C++ライブラリ対応 | ✅ 完了・動作確認済み |
| VSCode拡張 | ✅ 完了 |
| プリプロセッサ | ✅ 完了 |
| C風マクロ | ✅ 完了 |

**v0.13.0の主要機能は全て実装完了しています！**

## 次のステップ

1. より多くのFFI使用例の作成
2. パフォーマンステスト
3. エラーハンドリングの改善
4. ドキュメントの充実

## 参考資料

- `docs/FFI_GUIDE.md` - FFIの詳細ガイド
- `sample/ffi_cpp_example.cpp` - C++ライブラリの例
- `sample/ffi_cpp_example.cb` - Cb側の使用例
- `release_notes/v0.13.0.md` - リリースノート
