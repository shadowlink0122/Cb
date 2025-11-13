# Cb Language Support for Visual Studio Code

Cb プログラミング言語の Visual Studio Code 拡張機能です。シンタックスハイライトと言語サポートを提供します。

## 機能

- **シンタックスハイライト**: Cb言語の構文を完全にサポート
- **自動補完**: 括弧、引用符、丸括弧の自動閉じ
- **コード折りたたみ**: リージョンベースのコード折りたたみをサポート
- **コメントサポート**: 行コメント (`//`) とブロックコメント (`/* */`) の切り替え
- **ファイルアイコン**: `.cb` ファイル専用のカスタムアイコン

## サポートされている言語機能

### キーワードと制御フロー
- 制御構造: `if`, `else`, `for`, `while`, `break`, `continue`, `return`
- パターンマッチング: `match`, `case`, `switch`, `default`
- 高度な制御: `defer`, `yield`, `async`, `await`
- エラー処理: `try`, `checked`, `panic`, `unwrap`
- モジュールシステム: `import`, `export`, `private`

### データ型
- プリミティブ型: `tiny`, `short`, `int`, `long`, `float`, `double`, `char`, `string`, `bool`, `void`
- 型定義: `struct`, `enum`, `interface`, `typedef`, `union`, `impl`
- 型修飾子: `const`, `static`, `private`, `unsigned`

### 高度な機能
- ジェネリクスサポート
- Async/await構文
- 文字列補間 `{変数}`
- ポインタと参照
- 関数ポインタ
- `impl`によるメソッド実装

### プリプロセッササポート (v0.13.1+)
- **ディレクティブ**: `#define`, `#undef`, `#ifdef`, `#ifndef`, `#if`, `#elif`, `#elseif`, `#else`, `#endif`, `#error`, `#warning`, `#include`（ピンク色）
- **組み込みマクロ**: `__FILE__`, `__LINE__`, `__DATE__`, `__TIME__`, `__VERSION__`（定数色）

### シンタックスハイライト (v0.13.2+)
- **ストレージ修飾子**: `static`, `const`, `unsigned`（青色 - C++と同様）
- **プリミティブ型**: `int`, `long`, `float`, `double` など（青色）
- **定数**: 全て大文字の識別子 (例: `MAX_SIZE`, `PI_VALUE`) は定数として強調表示
- **数値リテラル**: 10進数、16進数、2進数、8進数、浮動小数点数（定数色）

## インストール方法

### 方法1: VSIXファイルからインストール（推奨）

1. [リリースページ](https://github.com/shadowlink0122/Cb/releases)から最新の `.vsix` ファイルをダウンロード
2. Visual Studio Code を起動
3. 拡張機能ビューを開く（Windows/Linuxは `Ctrl+Shift+X`、macOSは `Cmd+Shift+X`）
4. 右上の "**...**" メニューをクリック
5. "**Install from VSIX...**"（VSIXからのインストール）を選択
6. ダウンロードした `cb-language-x.x.x.vsix` ファイルを選択
7. プロンプトが表示されたらVS Codeをリロード

### 方法2: コマンドラインからインストール

```bash
code --install-extension cb-language-0.13.0.vsix
```

### 方法3: 手動インストール

1. 拡張機能フォルダをVS Codeの拡張機能ディレクトリにコピー:
   - **Windows**: `%USERPROFILE%\.vscode\extensions\cb-language-0.13.0`
   - **macOS/Linux**: `~/.vscode/extensions/cb-language-0.13.0`
2. VS Codeを再起動

## ソースからビルド

拡張機能を自分でビルドする場合:

```bash
# リポジトリをクローン
git clone https://github.com/shadowlink0122/Cb.git
cd Cb

# 拡張機能をビルド
make build-extension

# vscode-extension/ に .vsix ファイルが作成されます
# 上記のいずれかの方法でインストールしてください
```

## バージョン管理

この拡張機能のバージョンは、リポジトリルートの`.cbversion`ファイルと自動的に同期されます。

### 開発者向け

バージョンを更新する場合：

```bash
# 1. ルートの.cbversionファイルを更新
echo "0.14.0" > ../.cbversion

# 2. 拡張機能のバージョンを自動同期
npm run update-version

# 3. バージョンの一致を確認
npm run verify-version
```

詳細は [VERSION_MANAGEMENT.md](./VERSION_MANAGEMENT.md) を参照してください。

## 使用方法

インストール後、`.cb` ファイルを開くと自動的に拡張機能が有効になります。

## サンプルコード

```cb
// Cb言語のサンプル
import stdlib.std.time;

struct Point {
    int x;
    int y;
};

int distance(Point p1, Point p2) {
    int dx = p2.x - p1.x;
    int dy = p2.y - p1.y;
    return dx * dx + dy * dy;
}

async int compute(int n) {
    return n * 2;
}

void main() {
    Point p1 = {x: 0, y: 0};
    Point p2 = {x: 3, y: 4};

    int dist = distance(p1, p2);
    println("Distance squared: {dist}");

    int result = await compute(42);
    println("Result: {result}");
}
```

## 対応言語バージョン

この拡張機能はCb言語 **v0.13.0** をサポートしています。

## 貢献

貢献を歓迎します！[GitHubリポジトリ](https://github.com/shadowlink0122/Cb)にissueやプルリクエストをお気軽に送ってください。

## ライセンス

MIT

## リリースノート

### 0.13.0 (初回リリース)
- 完全なシンタックスハイライトサポートを含む初回リリース
- すべてのCb言語キーワードと構文のサポート
- 括弧、引用符、丸括弧の自動閉じ
- コード折りたたみサポート
- コメント切り替えサポート
- `.cb` ファイル用のカスタムアイコン
- import文のハイライト
