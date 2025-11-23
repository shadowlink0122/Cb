# .o Extension Implementation - Final Report

## 実施日
2024-11-16

## 概要
コンパイル済みバイナリの拡張子を`.o`に変更しました。macOS/Unixでは実行可能ファイルに拡張子は不要ですが、Cbでは`.o`を使用することで、他の言語（C/C++）と統一された管理が可能になります。

## 変更内容

### 1. .o拡張子の実装

#### デフォルトの出力先
コンパイルされたバイナリは、デフォルトで**元のファイルと同じディレクトリ**に`.o`拡張子で出力されます。

```bash
# 入力ファイル
sample/algorithm/knapsack.cb

# 出力ファイル（デフォルト）
sample/algorithm/knapsack.o
```

#### コード変更
`src/frontend/main.cpp`:
```cpp
// デフォルト: 入力ファイルと同じディレクトリに .o 拡張子で出力
output_binary = filename;
size_t dot_pos = output_binary.find_last_of('.');
if (dot_pos != std::string::npos) {
    output_binary = output_binary.substr(0, dot_pos);
}
output_binary += ".o";
```

### 2. C++オブジェクトファイルとの共存

#### 問題
`.o`はC++のオブジェクトファイル（中間ファイル）としても使用されています。

```
src/frontend/main.o         # C++オブジェクトファイル
tests/cases/basic/test.o    # Cb実行ファイル
```

#### 解決策
ディレクトリ別にクリーンアップを分離：

```makefile
# src/ のオブジェクトファイル（C++中間ファイル）
find src -name "*.o" -type f -delete

# tests/ と sample/ の実行ファイル（Cbコンパイル済み）
find tests -name "*.o" -type f -delete
find sample -name "*.o" -type f -delete
```

### 3. Makefileの改善

```makefile
clean: clean-ffi
@echo "Cleaning up build artifacts..."
rm -f $(MAIN_TARGET) $(CGEN_TARGET)
rm -f main_asan
rm -f tests/integration/test_main
rm -f tests/unit/test_main tests/unit/dummy.o
rm -f tests/stdlib/test_main
rm -f /tmp/cb_integration_test.log
find src -name "*.o" -type f -delete      # C++オブジェクト
find tests -name "*.o" -type f -delete    # Cb実行ファイル
find sample -name "*.o" -type f -delete   # Cb実行ファイル
rm -rf tmp/
rm -rf **/*.dSYM *.dSYM
rm -rf tests/integration/*.dSYM
rm -rf tests/unit/*.dSYM
rm -rf tests/stdlib/*.dSYM
@echo "Clean completed."
```

### 4. .gitignoreの更新

`.o`ファイルは既にバージョン管理から除外されていますが、Cbの実行ファイルを明示的に追加：

```gitignore
# Cb compiled executables
tests/**/*.o
sample/**/*.o
```

### 5. ヘルプメッセージの更新

```cpp
std::cout << "\nOutput:\n";
std::cout << "  Without -o: Creates <filename>.o in the same directory\n";
std::cout << "  With -o:    Creates executable with specified name\n";
std::cout << "  Debug mode: Keeps generated C++ code in ./tmp/ directory\n";
```

## 使用例

### 基本的な使用方法

#### 1. デフォルトの.o出力
```bash
$ ./cb compile tests/cases/basic/simple_main.cb
# 出力: tests/cases/basic/simple_main.o

$ ./cb -c sample/algorithm/fibonacci.cb
# 出力: sample/algorithm/fibonacci.o

$ ./tests/cases/basic/simple_main.o
# 実行可能
```

#### 2. -o オプションでカスタム出力
```bash
$ ./cb compile program.cb -o myapp
# 出力: myapp (拡張子なし)

$ ./cb -c program.cb -o /usr/local/bin/myapp
# 出力: /usr/local/bin/myapp
```

#### 3. デバッグモード
```bash
$ ./cb -c program.cb -d
# 出力: 
#   program.o (実行ファイル)
#   tmp/program.generated.cpp (デバッグ用C++コード)
```

### ディレクトリ構造

#### コンパイル前
```
project/
├── src/                   # C++ソースコード
│   └── frontend/
│       └── main.cpp
├── tests/
│   └── cases/
│       └── basic/
│           └── test.cb
└── sample/
    └── algorithm/
        └── fibonacci.cb
```

#### コンパイル後
```
project/
├── src/                   # C++ソースコード
│   └── frontend/
│       ├── main.cpp
│       └── main.o         # C++オブジェクトファイル（中間）
├── tests/
│   └── cases/
│       └── basic/
│           ├── test.cb
│           └── test.o     # Cb実行ファイル ✅
└── sample/
    └── algorithm/
        ├── fibonacci.cb
        └── fibonacci.o    # Cb実行ファイル ✅
```

### クリーンアップ

```bash
$ make clean

# 削除されるもの:
# - src/**/*.o         (C++オブジェクトファイル)
# - tests/**/*.o       (Cb実行ファイル)
# - sample/**/*.o      (Cb実行ファイル)
# - tmp/               (一時ファイル)
```

## メリット

### 1. 他の言語との統一
- ✅ C/C++と同じ`.o`拡張子
- ✅ 統一されたビルドシステム
- ✅ Makefileでの管理が容易

### 2. VSCodeでの問題解決
- ✅ `.cbx`はTEXファイルとして誤認識される問題を解決
- ✅ `.o`は一般的なバイナリとして正しく認識

### 3. 明確な管理
- ✅ ディレクトリ別に役割を分離
  - `src/` = C++中間ファイル（.o）
  - `tests/`, `sample/` = Cb実行ファイル（.o）

### 4. 整理されたディレクトリ
- ✅ ソースファイルと同じディレクトリに出力
- ✅ プロジェクト構造が自然
- ✅ バイナリの場所が分かりやすい

## テスト結果

### 機能テスト
```bash
✅ デフォルト出力: program.cb → program.o
✅ 異なるディレクトリ: sample/test.cb → sample/test.o
✅ -o オプション: program.cb -o myapp → myapp
✅ 実行可能: ./program.o が正常に動作
✅ C++ビルドに影響なし: src/*.o は中間ファイルとして機能
```

### クリーンアップテスト
```bash
✅ make clean で tests/**/*.o が削除
✅ make clean で sample/**/*.o が削除
✅ make clean で src/**/*.o も削除（再ビルド可能）
✅ tmp/ が削除
```

### 統合テスト
```bash
✅ 4373/4373 tests passed
✅ すべての既存機能が正常動作
```

## 比較: 他の言語ツール

| 言語 | ソース | オブジェクト | 実行ファイル | 管理 |
|------|--------|------------|------------|------|
| C | `.c` | `.o` | (なし) | `gcc -c`でオブジェクト、リンクで実行ファイル |
| C++ | `.cpp` | `.o` | (なし) | `g++ -c`でオブジェクト、リンクで実行ファイル |
| Rust | `.rs` | - | (なし) | `rustc`で直接実行ファイル |
| Go | `.go` | - | (なし) | `go build`で直接実行ファイル |
| **Cb** | `.cb` | - | `.o` | `cb compile`で直接実行ファイル |

Cbの`.o`は実行ファイルであり、C/C++のオブジェクトファイル（中間ファイル）とは異なります。

## 設計の妥当性

### なぜ .o を選んだか

1. **Unix/macOSの慣習**
   - 実行ファイルに拡張子は不要
   - しかし、VSCodeなどのエディタで管理しやすくするため拡張子が有用

2. **既存ツールとの統一**
   - C/C++と同じ拡張子で統一感
   - Makefileでの管理が自然

3. **ディレクトリ分離で共存**
   - `src/` = C++の中間ファイル
   - `tests/`, `sample/` = Cbの実行ファイル
   - 役割が明確で混乱しない

4. **VSCodeの問題解決**
   - `.cbx` = TEXファイルとして誤認識
   - `.o` = 一般的なバイナリとして正しく認識

## まとめ

この実装により：

1. ✅ **明確な命名**: `.o`で実行ファイルを識別
2. ✅ **VSCode対応**: TEXファイル誤認識の問題を解決
3. ✅ **統一感**: C/C++と同じ拡張子で管理
4. ✅ **共存**: C++オブジェクトファイルとディレクトリで分離
5. ✅ **簡単なクリーンアップ**: `make clean`で適切に管理

Cbコンパイラがより実用的で、他の言語ツールと統一されたツールになりました！
