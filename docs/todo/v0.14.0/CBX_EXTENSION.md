# .cbx Extension Implementation - Complete Report

## 実施日
2024-11-16

## 概要
コンパイル済みバイナリの拡張子を`.cbx`に変更し、出力先とクリーンアップの改善を実装しました。

## 変更内容

### 1. .cbx拡張子の実装

#### デフォルトの出力先
コンパイルされたバイナリは、デフォルトで**元のファイルと同じディレクトリ**に`.cbx`拡張子で出力されます。

```bash
# 入力ファイル
sample/algorithm/knapsack.cb

# 出力ファイル（デフォルト）
sample/algorithm/knapsack.cbx
```

#### コード変更
`src/frontend/main.cpp`:
```cpp
// 出力ファイル名を決定
std::string output_binary;
if (!output_file.empty()) {
    // -o オプションで指定された場合はそのまま使用
    output_binary = output_file;
} else {
    // デフォルト: 入力ファイルと同じディレクトリに .cbx 拡張子で出力
    output_binary = filename;
    size_t dot_pos = output_binary.find_last_of('.');
    if (dot_pos != std::string::npos) {
        output_binary = output_binary.substr(0, dot_pos);
    }
    output_binary += ".cbx";
}
```

### 2. -o オプションの動作

`-o`オプションで出力名を指定した場合は、その名前をそのまま使用します。

```bash
# -o オプション使用
$ ./cb compile program.cb -o myapp
# 出力: myapp (拡張子なし、指定通り)

$ ./cb -c program.cb -o /usr/local/bin/myapp
# 出力: /usr/local/bin/myapp (指定したパスとファイル名)
```

### 3. Makefileのcleanターゲット改善

#### 追加されたクリーンアップ対象
1. `*.cbx` - 再帰的にすべての.cbxファイルを削除
2. `tmp/` - 一時ディレクトリごと削除

```makefile
clean: clean-ffi
	@echo "Cleaning up build artifacts..."
	rm -f $(MAIN_TARGET) $(CGEN_TARGET)
	rm -f main_asan
	rm -f tests/integration/test_main
	rm -f tests/unit/test_main tests/unit/dummy.o
	rm -f tests/stdlib/test_main
	rm -f /tmp/cb_integration_test.log
	find . -name "*.o" -type f -delete
	find . -name "*.cbx" -type f -delete    # 追加
	rm -rf tmp/                              # 追加
	rm -rf **/*.dSYM *.dSYM
	rm -rf tests/integration/*.dSYM
	rm -rf tests/unit/*.dSYM
	rm -rf tests/stdlib/*.dSYM
	@echo "Clean completed."
```

### 4. .gitignoreの更新

`.cbx`ファイルをバージョン管理から除外：

```
*.cbx
tmp/
```

### 5. ヘルプメッセージの更新

`src/frontend/help_messages.cpp`:
```cpp
std::cout << "\nOutput:\n";
std::cout << "  Without -o: Creates <filename>.cbx in the same directory\n";
std::cout << "  With -o:    Creates executable with specified name\n";
std::cout << "  Debug mode: Keeps generated C++ code in ./tmp/ directory\n";
```

## 使用例

### 基本的な使用方法

#### 1. デフォルトの.cbx出力
```bash
$ ./cb compile tests/cases/basic/simple_main.cb
# 出力: tests/cases/basic/simple_main.cbx

$ ./cb -c sample/algorithm/fibonacci.cb
# 出力: sample/algorithm/fibonacci.cbx

$ tests/cases/basic/simple_main.cbx
# 実行可能
```

#### 2. -o オプションでカスタム出力
```bash
$ ./cb compile program.cb -o myapp
# 出力: myapp

$ ./cb -c program.cb -o /usr/local/bin/myapp
# 出力: /usr/local/bin/myapp
```

#### 3. デバッグモード
```bash
$ ./cb -c program.cb -d
# 出力: 
#   program.cbx (実行ファイル)
#   tmp/program.generated.cpp (デバッグ用C++コード)
```

### ディレクトリ構造の例

#### Before (コンパイル前)
```
sample/
├── algorithm/
│   ├── fibonacci.cb
│   ├── knapsack.cb
│   └── quicksort.cb
└── basic/
    └── hello.cb
```

#### After (コンパイル後)
```bash
$ ./cb -c sample/algorithm/fibonacci.cb
$ ./cb -c sample/algorithm/knapsack.cb
$ ./cb -c sample/basic/hello.cb
```

```
sample/
├── algorithm/
│   ├── fibonacci.cb
│   ├── fibonacci.cbx      # 生成
│   ├── knapsack.cb
│   ├── knapsack.cbx       # 生成
│   └── quicksort.cb
└── basic/
    ├── hello.cb
    └── hello.cbx          # 生成
```

### クリーンアップ

```bash
# すべての.cbxファイルとtmp/を削除
$ make clean

# 確認
$ find . -name "*.cbx"
# (何も表示されない)

$ ls tmp/
# ls: tmp/: No such file or directory
```

## メリット

### 1. 明確な識別
- ✅ `.cbx` 拡張子でCbバイナリであることが一目で分かる
- ✅ 他のバイナリと区別しやすい

### 2. 整理されたディレクトリ
- ✅ ソースファイルと同じディレクトリに出力される
- ✅ プロジェクト構造が自然
- ✅ バイナリの場所が分かりやすい

### 3. 簡単なクリーンアップ
- ✅ `make clean`で再帰的にすべて削除
- ✅ `tmp/`ディレクトリもまとめて削除
- ✅ プロジェクトを簡単にクリーンな状態に戻せる

### 4. 柔軟な出力先
- ✅ `-o`オプションで任意の場所に出力可能
- ✅ インストールスクリプトで便利

## テスト結果

### 機能テスト
```bash
✅ デフォルト出力: program.cb → program.cbx
✅ 異なるディレクトリ: sample/test.cb → sample/test.cbx
✅ -o オプション: program.cb -o myapp → myapp
✅ 実行可能: ./program.cbx が正常に動作
```

### クリーンアップテスト
```bash
✅ make clean で *.cbx が削除される
✅ make clean で tmp/ が削除される
✅ 再帰的にすべてのディレクトリから削除
```

### 統合テスト
```bash
✅ 4373/4373 tests passed
✅ すべての既存機能が正常動作
```

## 比較: 他の言語ツール

Cbの`.cbx`拡張子は、他の言語ツールと一貫性があります：

| 言語 | ソースファイル | コンパイル済み | コマンド例 |
|------|--------------|--------------|-----------|
| Rust | `.rs` | (バイナリ) | `rustc program.rs` |
| Go | `.go` | (バイナリ) | `go build program.go` |
| Java | `.java` | `.class` | `javac Program.java` |
| Python | `.py` | `.pyc` | `python -m py_compile` |
| **Cb** | `.cb` | `.cbx` | `cb compile program.cb` |

## 将来の拡張可能性

### 1. インストールコマンド
```bash
# 将来的に実装可能
$ cb install program.cb
# /usr/local/bin/program にインストール
```

### 2. パッケージ管理
```bash
# プロジェクトのビルド
$ cb build
# すべての.cbファイルを.cbxにコンパイル

# クリーン
$ cb clean
# すべての.cbxを削除
```

### 3. デプロイメント
```bash
# .cbxファイルのみをデプロイ
$ find . -name "*.cbx" | xargs -I {} cp {} /deploy/
```

## まとめ

この実装により：

1. ✅ **明確な命名**: `.cbx`でコンパイル済みバイナリを識別
2. ✅ **直感的な配置**: ソースと同じディレクトリに出力
3. ✅ **簡単なクリーンアップ**: `make clean`で一括削除
4. ✅ **柔軟な出力**: `-o`オプションで自由に指定可能

Cbコンパイラがより使いやすく、整理されたツールになりました！
