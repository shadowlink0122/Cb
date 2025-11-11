# Cbプレゼンテーション構造

## 📁 ディレクトリ構造

```
presentation/
├── 📄 presentation.html          # ★ メインファイル（ビルド済み）
├── 🔧 build.sh                    # ビルドスクリプト
├── 📖 README_MODULAR.md          # 使い方ガイド
├── 📋 SLIDES_INDEX.md            # スライド一覧
│
├── 📁 css/
│   └── styles.css                # 全スタイル定義
│
├── 📁 sections/                  # ★ 各スライドの編集はここ
│   ├── slide_001.html           # タイトルスライド
│   ├── slide_002.html           # 自己紹介
│   ├── slide_003.html           # Cbとは
│   ├── ...                      # （全55スライド）
│   └── slide_055.html           # 終了スライド
│
├── 📁 assets/                    # 画像などのリソース
├── 📁 js/                        # JavaScriptファイル
│
└── 📁 old/                       # バックアップ
    ├── cb_interpreter_presentation.html  # 元のファイル
    └── index.html                        # 古いバージョン
```

## 🚀 クイックスタート

### 1. スライドを編集

```bash
# 任意のスライドを編集
vim sections/slide_010.html
```

### 2. プレゼンテーションをビルド

```bash
./build.sh
```

### 3. 確認

```bash
# ローカルサーバーを起動
python3 -m http.server 8000

# ブラウザで開く
open http://localhost:8000/presentation.html
```

## ✏️ 編集例

### スライドタイトルを変更

```bash
# slide_003.htmlを編集
vim sections/slide_003.html

# <h2>タグの内容を変更
# 変更前: <h2>Cbとは？</h2>
# 変更後: <h2>Cbの概要</h2>
```

### CSSスタイルを変更

```bash
# スタイルを編集
vim css/styles.css

# 例: タイトルサイズを変更
# .reveal h1 { font-size: 2.0em; ... }
```

### スライドを追加

```bash
# 新しいスライドを作成
cat > sections/slide_056.html << 'SLIDE'
<section>
    <h2>新しいスライド</h2>
    <p>内容をここに記述</p>
</section>
SLIDE

# ビルド
./build.sh
```

## 📊 スライド構成

- **Slide 001-007**: イントロダクション
- **Slide 008-024**: Section 1 - Cbの機能紹介
- **Slide 025-033**: Section 2 - バイブコーディング
- **Slide 034-039**: Section 3 - 実装詳細
- **Slide 040-050**: Section 4 - 内部実装
- **Slide 051-055**: まとめ・クロージング

詳細は `SLIDES_INDEX.md` を参照してください。

## 🔧 メンテナンス

### スライド一覧を更新

```bash
./update_index.sh
```

### 全スライドでテキスト置換

```bash
# CB → Cb に一括置換
sed -i '' 's/CB/Cb/g' sections/*.html
./build.sh
```

### 特定のスライドを検索

```bash
# "async/await"を含むスライドを検索
grep -l "async/await" sections/*.html
```

## 💡 Tips

- 各スライドは独立したファイルなので、GitHubでの差分確認が容易
- CSSを一元管理することでデザイン変更が簡単
- ビルドスクリプトで自動結合されるため、最終成果物は1ファイル
- スライドの順序は連番で管理（リネームで順序変更可能）

## 📝 注意事項

- `presentation.html` は自動生成されるため、直接編集しない
- 編集は必ず `sections/` と `css/` で行う
- ビルド後は必ずブラウザで確認する
