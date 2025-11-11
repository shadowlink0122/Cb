# Cbプレゼンテーション - モジュラー構造

このプレゼンテーションは、メンテナンス性を向上させるためにモジュラー構造に分割されています。

## ディレクトリ構造

```
presentation/
├── css/
│   └── styles.css          # すべてのカスタムスタイル
├── sections/
│   ├── slide_001.html      # 各スライドの個別ファイル
│   ├── slide_002.html
│   └── ...
├── js/
│   └── load-slides.js      # スライド読み込みスクリプト
├── assets/                 # 画像などのアセット
├── build.sh                # ビルドスクリプト
├── presentation.html       # ビルド済みのプレゼンテーション（単一ファイル）
└── README_MODULAR.md       # このファイル
```

## 使い方

### 1. スライドを編集する

`sections/slide_XXX.html` ファイルを直接編集します。
各ファイルには1つの `<section>` タグが含まれています。

例：
```bash
# 最初のスライドを編集
vim sections/slide_001.html
```

### 2. CSSを編集する

`css/styles.css` ファイルを編集します。

### 3. プレゼンテーションをビルドする

```bash
./build.sh
```

これにより、すべてのスライドとCSSが結合され、`presentation.html` が生成されます。

### 4. プレゼンテーションを表示する

```bash
# ローカルサーバーを起動（推奨）
python3 -m http.server 8000
# ブラウザで http://localhost:8000/presentation.html を開く
```

または直接ブラウザで `presentation.html` を開きます。

## スライドの追加・削除

### スライドを追加

1. `sections/` ディレクトリに新しいファイルを作成
   - ファイル名: `slide_XXX.html`（連番）
2. `<section>` タグでコンテンツを囲む
3. `./build.sh` を実行

### スライドを削除

1. `sections/` から該当ファイルを削除
2. `./build.sh` を実行

### スライドの順序を変更

1. ファイル名の番号を変更（例: `slide_010.html` → `slide_005.html`）
2. `./build.sh` を実行

## 便利なコマンド

```bash
# スライド数をカウント
ls -1 sections/slide_*.html | wc -l

# 特定のキーワードを含むスライドを検索
grep -l "async/await" sections/*.html

# すべてのスライドで置換
sed -i '' 's/CB/Cb/g' sections/*.html
```

## 元のファイル

- `cb_interpreter_presentation.html`: バックアップ（元のファイル）
- `cb_interpreter_presentation.html.old`: さらに古いバックアップ

## トラブルシューティング

### スタイルが適用されない

- `css/styles.css` が正しく配置されているか確認
- `./build.sh` を再実行

### スライドが表示されない

- `sections/` 内のHTMLファイルが正しい形式か確認
- `<section>` タグが正しく閉じられているか確認

### ビルドエラー

- シェルスクリプトに実行権限があるか確認: `chmod +x build.sh`
- ファイルパスが正しいか確認
