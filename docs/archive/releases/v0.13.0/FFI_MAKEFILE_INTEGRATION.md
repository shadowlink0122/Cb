# FFI サンプルとMakefile統合 - 実装完了レポート

**日付**: 2025-11-14
**ステータス**: ✅ 完了

## 概要

FFIサンプルをディレクトリに整理し、Makefileにビルドルールを追加しました。

## 実装内容

### 1. ディレクトリ構造の整理

#### Before (変更前)
```
sample/
├── ffi_cpp_example.cpp
├── ffi_cpp_example.cb
├── advanced_cpp_ffi.cpp
├── advanced_cpp_ffi.cb
└── (他のサンプル)
```

#### After (変更後)
```
sample/
├── ffi/                          # 新規ディレクトリ
│   ├── README.md                 # FFIサンプルのドキュメント
│   ├── ffi_cpp_example.cpp       # 基本的なC++例
│   ├── ffi_cpp_example.cb        # Cb側の使用例
│   ├── advanced_cpp_ffi.cpp      # 高度なC++例
│   └── advanced_cpp_ffi.cb       # Cb側の使用例
└── (他のサンプル)
```

**利点**:
- FFI関連ファイルが一箇所に集約
- 見つけやすく管理しやすい
- 将来的にFFIサンプルを追加しやすい

### 2. Makefileの拡張

#### 追加された変数

```makefile
# ディレクトリ
SAMPLE_FFI_DIR=sample/ffi
STDLIB_FOREIGN_DIR=stdlib/foreign

# FFIライブラリ設定
FFI_LIBS=$(STDLIB_FOREIGN_DIR)/libcppexample.$(LIB_EXT) \
         $(STDLIB_FOREIGN_DIR)/libadvanced.$(LIB_EXT)

# OSごとのライブラリ拡張子
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    LIB_EXT=dylib
    FFI_CC=clang++
else ifeq ($(UNAME_S),Linux)
    LIB_EXT=so
    FFI_CC=g++
else
    LIB_EXT=dll
    FFI_CC=g++
endif
```

#### 追加されたターゲット

1. **`ffi-libs`** - FFIライブラリをビルド
   ```bash
   make ffi-libs
   ```
   
2. **`clean-ffi`** - FFIライブラリを削除
   ```bash
   make clean-ffi
   ```
   
3. **`test-ffi`** - FFIサンプルをテスト
   ```bash
   make test-ffi
   ```

#### ビルドルール

```makefile
# ffi_cpp_example ライブラリのビルド
$(STDLIB_FOREIGN_DIR)/libcppexample.$(LIB_EXT): $(SAMPLE_FFI_DIR)/ffi_cpp_example.cpp
	@echo "Compiling FFI library: ffi_cpp_example..."
	$(FFI_CC) -std=c++17 -shared -fPIC $< -o $@
	@echo "  ✓ Built: $@"

# advanced_cpp_ffi ライブラリのビルド
$(STDLIB_FOREIGN_DIR)/libadvanced.$(LIB_EXT): $(SAMPLE_FFI_DIR)/advanced_cpp_ffi.cpp
	@echo "Compiling FFI library: advanced_cpp_ffi..."
	$(FFI_CC) -std=c++17 -shared -fPIC $< -o $@
	@echo "  ✓ Built: $@"
```

#### 変更されたターゲット

1. **`all`** - FFIライブラリも自動ビルド
   ```makefile
   all: setup-dirs $(MAIN_TARGET) ffi-libs
   ```

2. **`clean`** - FFIライブラリも削除
   ```makefile
   clean: clean-ffi
   ```

3. **`setup-dirs`** - FFIディレクトリも作成
   ```makefile
   setup-dirs:
       # ...
       @mkdir -p $(STDLIB_FOREIGN_DIR)
       @mkdir -p $(SAMPLE_FFI_DIR)
   ```

4. **`help`** - FFIコマンドを追加
   ```
   FFI (Foreign Function Interface):
     ffi-libs               - Build all FFI example libraries
     test-ffi               - Run FFI example tests
     clean-ffi              - Remove compiled FFI libraries
   ```

### 3. プラットフォーム対応

| プラットフォーム | 拡張子 | コンパイラ | 自動検出 |
|-----------------|--------|----------|---------|
| macOS | `.dylib` | `clang++` | ✅ |
| Linux | `.so` | `g++` | ✅ |
| Windows | `.dll` | `g++` | ✅ |

### 4. ドキュメント

#### 新規作成: `sample/ffi/README.md`

**内容**:
- サンプルの説明
- ビルド方法（手動 & Makefile）
- 実行方法
- 出力例
- 技術的な詳細
- トラブルシューティング

**文字数**: 約4,600文字

## 動作確認

### テスト1: クリーンビルド

```bash
$ make clean
Cleaning FFI libraries...
FFI libraries cleaned.
Cleaning up build artifacts...
Clean completed.

$ make ffi-libs
Compiling FFI library: ffi_cpp_example...
clang++ -std=c++17 -shared -fPIC sample/ffi/ffi_cpp_example.cpp -o stdlib/foreign/libcppexample.dylib
  ✓ Built: stdlib/foreign/libcppexample.dylib
Compiling FFI library: advanced_cpp_ffi...
clang++ -std=c++17 -shared -fPIC sample/ffi/advanced_cpp_ffi.cpp -o stdlib/foreign/libadvanced.dylib
  ✓ Built: stdlib/foreign/libadvanced.dylib
FFI libraries built successfully.
```

**結果**: ✅ 成功

### テスト2: 依存関係

```bash
# ソースファイルを変更
$ touch sample/ffi/ffi_cpp_example.cpp

# 自動再ビルド
$ make ffi-libs
Compiling FFI library: ffi_cpp_example...
clang++ -std=c++17 -shared -fPIC sample/ffi/ffi_cpp_example.cpp -o stdlib/foreign/libcppexample.dylib
  ✓ Built: stdlib/foreign/libcppexample.dylib
FFI libraries built successfully.
```

**結果**: ✅ 成功 - 変更されたファイルのみ再ビルド

### テスト3: clean-ffi

```bash
$ make clean-ffi
Cleaning FFI libraries...
rm -f stdlib/foreign/*.dylib
rm -f stdlib/foreign/*.so
rm -f stdlib/foreign/*.dll
FFI libraries cleaned.

$ ls stdlib/foreign/*.dylib
ls: stdlib/foreign/*.dylib: No such file or directory
```

**結果**: ✅ 成功

### テスト4: ヘルプ表示

```bash
$ make help | grep -A 3 "FFI"
FFI (Foreign Function Interface):
  ffi-libs               - Build all FFI example libraries
  test-ffi               - Run FFI example tests
  clean-ffi              - Remove compiled FFI libraries
```

**結果**: ✅ 成功

## 技術的な詳細

### Make変数の評価順序

```makefile
# 1. OSを検出
UNAME_S := $(shell uname -s)

# 2. 拡張子とコンパイラを設定
ifeq ($(UNAME_S),Darwin)
    LIB_EXT=dylib
    FFI_CC=clang++
endif

# 3. FFI_LIBSで使用
FFI_LIBS=$(STDLIB_FOREIGN_DIR)/libcppexample.$(LIB_EXT)
```

この順序により、プラットフォームに応じた正しい拡張子が使用されます。

### パターンルール

```makefile
$(STDLIB_FOREIGN_DIR)/libcppexample.$(LIB_EXT): $(SAMPLE_FFI_DIR)/ffi_cpp_example.cpp
```

- **ターゲット**: `stdlib/foreign/libcppexample.dylib` (macOS)
- **依存関係**: `sample/ffi/ffi_cpp_example.cpp`
- **自動変数**:
  - `$<`: 最初の依存関係 (`ffi_cpp_example.cpp`)
  - `$@`: ターゲット (`libcppexample.dylib`)

### クリーンアップの順序

```makefile
clean: clean-ffi
    # その他のクリーンアップ
```

`clean-ffi`を先に実行することで、FFIライブラリが確実に削除されます。

## ベストプラクティス

### 1. ディレクトリ構造

✅ **Good**: `sample/ffi/` - 関連ファイルをグループ化
❌ **Bad**: `sample/` - 全てが混在

### 2. Makefileの整理

✅ **Good**: 変数で設定を集約
```makefile
SAMPLE_FFI_DIR=sample/ffi
FFI_CC=clang++
```

❌ **Bad**: ハードコード
```makefile
clang++ sample/ffi_cpp_example.cpp -o stdlib/foreign/libcppexample.dylib
```

### 3. プラットフォーム対応

✅ **Good**: 自動検出
```makefile
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    LIB_EXT=dylib
endif
```

❌ **Bad**: 手動切り替え

### 4. ドキュメント

✅ **Good**: READMEに詳細な説明
✅ **Good**: `make help`にコマンドを追加

## 今後の拡張

### 追加可能なFFIサンプル

1. **Rustライブラリ統合**
   - `sample/ffi/rust_example/`
   - Cargo.toml設定
   - `#[no_mangle]` + `extern "C"`

2. **Zigライブラリ統合**
   - `sample/ffi/zig_example/`
   - `export fn` の使用例

3. **Goライブラリ統合**
   - `sample/ffi/go_example/`
   - `//export` コメント

### Makefileの拡張

```makefile
# Rustライブラリのビルド
$(STDLIB_FOREIGN_DIR)/librust_example.$(LIB_EXT):
	cd sample/ffi/rust_example && cargo build --release
	cp sample/ffi/rust_example/target/release/lib*.$(LIB_EXT) $@
```

## まとめ

### 達成したこと

| 項目 | ステータス |
|------|----------|
| FFIディレクトリ整理 | ✅ 完了 |
| Makefileビルドルール | ✅ 完了 |
| プラットフォーム対応 | ✅ 完了 |
| ドキュメント作成 | ✅ 完了 |
| 動作確認 | ✅ 完了 |

### 利点

1. **管理性の向上**: FFIサンプルが一箇所に集約
2. **自動化**: `make ffi-libs`で簡単にビルド
3. **クリーンアップ**: `make clean`で自動削除
4. **プラットフォーム対応**: macOS/Linux/Windowsで自動切り替え
5. **拡張性**: 新しいFFIサンプルを簡単に追加可能

### ファイル変更

| ファイル | 変更内容 |
|---------|---------|
| `Makefile` | FFIビルドルール追加（約50行） |
| `sample/ffi/README.md` | 新規作成 |
| `sample/ffi/*.cpp` | ディレクトリ移動 |
| `sample/ffi/*.cb` | ディレクトリ移動 |

### 使い方

```bash
# 一般的な使用法
make ffi-libs      # FFIライブラリをビルド
make clean-ffi     # FFIライブラリを削除
make clean         # 全てをクリーンアップ
make help          # ヘルプを表示

# 開発時
touch sample/ffi/ffi_cpp_example.cpp
make ffi-libs      # 変更されたファイルのみ再ビルド
```

---

**実装完了: FFIサンプルの整理とMakefile統合** ✅

🗂️  ディレクトリ構造の整理
⚙️  自動ビルドシステム
🌍 マルチプラットフォーム対応
📚 充実したドキュメント
