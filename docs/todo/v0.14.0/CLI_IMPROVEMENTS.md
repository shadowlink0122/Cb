# Cb Compiler CLI Improvements - v0.14.0

## 実装日
2024-11-16

## 変更内容

### 1. コマンドの短縮形サポート

#### 実装内容
- `run` → `-r` の省略形を追加
- `compile` → `-c` の省略形を追加

#### 使用例
```bash
# 従来
./cb run program.cb
./cb compile program.cb -o myapp

# 短縮形
./cb -r program.cb
./cb -c program.cb -o myapp
```

### 2. ヘルプシステムの強化

#### グローバルヘルプ
```bash
./cb --help
./cb -h
```
全体的な使い方とコマンド一覧を表示

#### コマンド別ヘルプ
```bash
./cb run --help      # インタプリタのヘルプ
./cb -r --help       # 同上（短縮形）

./cb compile --help  # コンパイラのヘルプ
./cb -c --help       # 同上（短縮形）
```

各コマンドに特化した詳細なヘルプを表示

### 3. バージョン表示

```bash
./cb --version
./cb -v
```

出力例：
```
Cb programming language version 0.14.0
Copyright (c) 2024 Cb Project
```

### 4. 一時ファイルの保存先変更

#### 変更前
- `/tmp/cb_compiled_*.cpp` - システムの一時ディレクトリ
- デバッグ時：カレントディレクトリに`<filename>.generated.cpp`

#### 変更後
- `./tmp/cb_compiled_*.cpp` - プロジェクトローカルの一時ディレクトリ
- デバッグ時：`./tmp/<basename>.generated.cpp`

#### メリット
1. **デバッグの容易さ**: 生成されたC++コードがプロジェクト内に保存される
2. **セキュリティ**: システム全体の`/tmp`ではなく、プロジェクトローカル
3. **クリーンアップの容易さ**: `./tmp/`ディレクトリを削除するだけ
4. **バージョン管理**: `.gitignore`に`tmp/`を追加済み

### 5. ヘルプメッセージの改善

#### 改善点
- より明確な説明
- 実用的な例を追加
- コマンドとオプションを分離
- 視覚的に見やすいフォーマット

#### 例：コンパイルヘルプ
```
Cb Compile Command - Compile Cb programs to native binaries

Usage: ./cb compile [options] <file>
   or: ./cb -c [options] <file>

Options:
  -o <output>             Specify output file name
  -d, --debug             Enable debug mode (keep generated C++)
  --debug-ja              Enable Japanese debug mode
  --no-preprocess         Disable preprocessor
  -D<macro>[=val]         Define preprocessor macro
  --help                  Show this help message

Examples:
  ./cb compile program.cb
  ./cb compile program.cb -o myapp
  ./cb -c program.cb -o myapp -d

Output:
  Without -o: Creates executable with same name as input file
  With -o:    Creates executable with specified name
  Debug mode: Keeps generated C++ code in ./tmp/ directory
```

## 使用例

### 基本的な使い方

#### インタプリタモード
```bash
# 標準
./cb run program.cb

# 短縮形
./cb -r program.cb

# デバッグモード
./cb -r program.cb -d
```

#### コンパイラモード
```bash
# 標準（入力ファイル名と同じ名前の実行ファイルを生成）
./cb compile program.cb

# 短縮形
./cb -c program.cb

# 出力ファイル名を指定
./cb -c program.cb -o myapp

# デバッグモード（生成されたC++を保存）
./cb -c program.cb -o myapp -d
```

### デバッグワークフロー

#### 1. デバッグモードでコンパイル
```bash
./cb -c program.cb -d
```

#### 2. 生成されたC++コードを確認
```bash
cat ./tmp/program.generated.cpp
```

#### 3. C++コードを直接編集してテスト
```bash
g++ -std=c++17 ./tmp/program.generated.cpp -o test_program
./test_program
```

#### 4. 一時ファイルをクリーンアップ
```bash
rm -rf ./tmp/
```

## ファイル変更

### 変更ファイル
- `src/frontend/main.cpp`
  - バージョン定義追加: `CB_VERSION`
  - `print_version()` 関数追加
  - `print_usage()` 関数改善
  - `print_run_help()` 関数追加
  - `print_compile_help()` 関数追加
  - コマンド解析に短縮形サポート追加
  - 一時ファイル保存先を`./tmp/`に変更
  - デバッグモード時のファイル名改善

- `.gitignore`
  - `tmp/` ディレクトリを追加

## テスト結果

### 機能テスト
```bash
# ヘルプ表示
✅ ./cb --help
✅ ./cb -h
✅ ./cb run --help
✅ ./cb compile --help

# バージョン表示
✅ ./cb --version
✅ ./cb -v

# 短縮形コマンド
✅ ./cb -r program.cb
✅ ./cb -c program.cb

# デバッグモード
✅ ./cb -c program.cb -d
✅ ./tmp/program.generated.cpp が生成される
```

### 統合テスト
```
✅ 4373/4373 integration tests passed
```

## まとめ

この改善により、Cb言語のCLIインターフェースは以下の点で向上しました：

1. **使いやすさ**: 短縮形により素早くコマンド実行が可能
2. **発見可能性**: 充実したヘルプシステムで学習が容易
3. **デバッグ性**: 生成されたコードをプロジェクト内で確認・編集可能
4. **プロフェッショナル**: バージョン情報とヘルプが標準的な形式

これにより、Cbは他の現代的なプログラミング言語ツール（Rust, Go, Node.jsなど）と同様の使い勝手を提供できるようになりました。
