# Cbプレゼンテーション

バイブコーディングで作る自作言語Cbインタプリタのプレゼンテーション資料です。

## 📁 プロジェクト構造（モジュラー構成）

このプレゼンテーションは**メンテナンス性向上のため**、CSSとスライドごとに分割されています。

```
presentation/
├── 📄 presentation.html               # ★ メインファイル（ビルド済み・配布用）
├── 🔧 build.sh                        # ビルドスクリプト
│
├── 📁 sections/                       # ★ 各スライドを編集（55ファイル）
│   ├── slide_001.html                # タイトルスライド
│   ├── slide_002.html                # 自己紹介
│   ├── slide_003.html                # Cbとは
│   └── ...                           # （全55スライド）
│
├── 📁 css/
│   └── styles.css                    # ★ 全スタイル定義を編集
│
├── 📁 js/                            # JavaScriptファイル
├── 📁 assets/                        # 画像などのリソース
├── 📁 old/                           # バックアップファイル
│
├── 📋 SLIDES_INDEX.md                # スライド一覧
├── 📖 README_STRUCTURE.md            # 詳細な構造説明
└── profile.jpg                       # プロフィール画像
```

### 🎯 編集の流れ

1. **スライド編集**: `sections/slide_XXX.html` を編集
2. **スタイル編集**: `css/styles.css` を編集  
3. **ビルド**: `./build.sh` を実行
4. **確認**: `presentation.html` をブラウザで開く

詳細は **`README_STRUCTURE.md`** を参照してください。

## 🚀 クイックスタート

### ローカルでの表示方法

#### 方法1: ブラウザで直接開く

```bash
open presentation.html
```

#### 方法2: Pythonのhttpサーバーを使用（推奨）

```bash
cd presentation
python3 -m http.server 8000
```

ブラウザで `http://localhost:8000/presentation.html` を開く

#### 方法3: VSCode Live Server拡張機能を使用

1. VSCodeで `presentation.html` を開く
2. 右クリック → "Open with Live Server"

### スライドの編集方法

```bash
# 1. スライドを編集
vim sections/slide_010.html

# 2. CSSを編集（必要に応じて）
vim css/styles.css

# 3. ビルド
./build.sh

# 4. 確認
open presentation.html
```

詳細な編集方法は **`README_STRUCTURE.md`** を参照してください。

## 🎮 操作方法

- **次のスライド**: 右矢印キー、スペースキー
- **前のスライド**: 左矢印キー
- **スライド番号**: 画面右下に表示
- **プログレスバー**: 画面下部に表示

## 📝 スライド構成（全55スライド）

### イントロ（7スライド）
- タイトル / 自己紹介 / Cbとは / 動機 / 課題と解決策 / 目標 / 構文比較

### Section 1: 実装済み機能（16スライド）
- Interface/Impl / Interface型 / FizzBuzz実例
- コンストラクタ/デストラクタ/defer
- カプセル化 / モジュールシステム
- パターンマッチング / メモリ管理 / async/await
- コード比較 / まとめ / 最適化 / バージョン履歴

### Section 2: バイブコーディング（9スライド）
- バイブコーディングとは / 開発ツール / 開発サイクル
- デバッグモード / 実装例 / コツと注意点
- ベストプラクティス / 魅力

### Section 3: 実装詳細（6スライド）
- ジェネリクス / 非同期処理 / Event Loop
- 非同期パターン / Result+async統合

### Section 4: インタプリタ内部（11スライド）
- Interface/Implの実装 / 協調的マルチタスク実装
- Result統合実装 / 技術スタック / プロジェクト構造
- 内部実装詳細 / sleepの非同期実装

### エンディング（6スライド）
- ロードマップ / 使ってみてください / 学んだこと / まとめ / 終了

詳細は **`SLIDES_INDEX.md`** を参照してください。

## 🛠️ カスタマイズ

### プロフィール画像の変更

`profile.jpg` を別の画像に置き換えてください（正方形推奨、2000x2000px）。

### スライドの追加・削除

```bash
# スライドを追加
cat > sections/slide_056.html << 'EOF'
<section>
    <h2>新しいスライド</h2>
    <p>内容</p>
</section>
EOF

# スライドを削除
rm sections/slide_010.html

# ビルド
./build.sh
```

### 一括テキスト置換

```bash
# 全スライドで"CB"を"Cb"に置換
sed -i '' 's/CB/Cb/g' sections/*.html
./build.sh
```

## 💻 技術スタック

- **Reveal.js 4.5.0**: プレゼンテーションフレームワーク
- **Highlight.js**: コードシンタックスハイライト
- **カスタムCSS**: VSCode Dark Theme風のスタイル（`css/styles.css`）

## ⏱️ 発表時間

全体で約20分の構成になっています。

## 📚 ドキュメント

- **README_STRUCTURE.md**: 詳細な構造とメンテナンス方法
- **README_MODULAR.md**: モジュラー構造の使い方ガイド
- **SLIDES_INDEX.md**: 全スライドのタイトル一覧

## 📄 ライセンス

このプレゼンテーションはCbプロジェクトの一部です。
