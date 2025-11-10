# クイックスタートガイド

最速でPDFを生成してGoogle Slidesに変換する方法です。

## 🚀 1分でPDF生成（Mac推奨）

### ステップ1: エクスポートスクリプトを実行

```bash
cd presentation
./export.sh
```

または

```bash
cd presentation
npm install
npm run export:pdf
```

### ステップ2: Keynoteで開く

1. **Keynote起動**
2. **ファイル → 開く**
3. **`cb_presentation.pdf` を選択**

### ステップ3: Google Slidesに変換

1. **Keynote で「ファイル → 書き出す → Keynote」**
2. **`cb_presentation.key` として保存**
3. **Google Drive にアップロード**
4. **ダブルクリック → 「Googleスライドで開く」**

完了！ 🎉

---

## 📋 詳細な手順

### 前提条件

- **Node.js**: v14以降（[nodejs.org](https://nodejs.org/)からインストール）
- **Mac**: Keynote利用の場合
- **インターネット接続**: 初回の依存関係インストール時

### Node.jsのインストール確認

```bash
node --version
# v18.0.0 などと表示されればOK
```

インストールされていない場合：
- https://nodejs.org/ から最新版をダウンロード
- または Homebrew: `brew install node`

---

## 方法1: インタラクティブスクリプト（推奨）✨

```bash
cd presentation
./export.sh
```

**メニューが表示されます:**
```
Select export format:
  1) PDF (高品質PDF - Keynote/Google Slides用)
  2) Images (各スライドをPNG画像として保存)
  3) Both (PDFと画像の両方)

Enter your choice (1-3):
```

**「1」を選択** → PDFが生成されます

**出力:**
- `cb_presentation.pdf` - 25ページのPDFファイル

---

## 方法2: npmコマンド直接実行

```bash
cd presentation

# 初回のみ: 依存関係をインストール
npm install

# PDFを生成
npm run export:pdf

# または画像を生成
npm run export:images

# または両方
npm run export:all
```

---

## 方法3: Node.jsスクリプト直接実行

```bash
cd presentation
npm install
node export_to_pdf_hq.js
```

---

## トラブルシューティング

### ❌ `command not found: node`

**原因**: Node.jsがインストールされていない

**解決策**:
```bash
# Homebrewを使用（推奨）
brew install node

# または公式サイトからダウンロード
# https://nodejs.org/
```

### ❌ `npm install` が失敗する

**解決策1**: キャッシュをクリア
```bash
npm cache clean --force
npm install
```

**解決策2**: node_modulesを削除して再インストール
```bash
rm -rf node_modules package-lock.json
npm install
```

### ❌ `Error: Failed to launch the browser process`

**原因**: Puppeteerのブラウザダウンロードが失敗

**解決策**:
```bash
# Puppeteerを再インストール
npm uninstall puppeteer
npm install puppeteer
```

### ❌ PDFが真っ白/スライドが表示されない

**原因**: reveal.jsの読み込みタイムアウト

**解決策**: スクリプト内のタイムアウトを増やす
```javascript
// export_to_pdf_hq.js の
timeout: 30000  // → 60000 に変更
```

### ❌ Keynoteが見つからない

**確認**:
```bash
open -a Keynote
```

**インストール**:
1. App Storeを開く
2. "Keynote"を検索
3. 「入手」をクリック（無料）

---

## 生成されるファイル

### PDFエクスポート
```
presentation/
  cb_presentation.pdf  ← 25ページのPDF
```

### 画像エクスポート
```
presentation/
  slides/
    slide_01.png  ← スライド1
    slide_02.png  ← スライド2
    ...
    slide_25.png  ← スライド25
```

---

## Google Slidesへの変換フロー

### フロー1: Keynote経由（Mac・推奨）

```
PDF生成 → Keynote → .key → Google Slides
   ↓         ↓        ↓         ↓
  npm      開く    書き出す  アップロード
```

**所要時間**: 約3分

### フロー2: 画像経由（品質重視）

```
画像生成 → Google Slides → 画像を挿入
   ↓           ↓              ↓
  npm       新規作成      25枚インポート
```

**所要時間**: 約10分

### フロー3: PDF直接（最速）

```
PDF生成 → Google Slides → インポート
   ↓           ↓              ↓
  npm      ファイル→     すべてインポート
                インポート
```

**所要時間**: 約1分
**注意**: 編集不可（画像として扱われる）

---

## ベストプラクティス

### 見た目重視の発表用
→ **画像エクスポート**を使用

### 後で編集したい場合
→ **Keynote経由**を使用

### とにかく速く
→ **PDF直接インポート**を使用

### バックアップ用
→ **両方エクスポート**（`npm run export:all`）

---

## 次のステップ

詳細な情報は以下のドキュメントを参照：

- **`README.md`** - 全体の概要と詳細手順
- **`KEYNOTE_GUIDE.md`** - Keynote専用ガイド（Mac）
- **`package.json`** - 利用可能なnpmスクリプト一覧

---

## 便利なコマンド一覧

```bash
# PDFのみ生成
npm run export:pdf

# 画像のみ生成
npm run export:images

# 両方生成
npm run export:all

# インタラクティブスクリプト
./export.sh

# Keynoteで開く（Mac）
open cb_presentation.pdf

# Google Driveフォルダを開く（ブラウザ）
open https://drive.google.com
```

---

**🎉 これで準備完了です！**

プレゼンテーションを楽しんでください！
