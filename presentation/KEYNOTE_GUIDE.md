# Keynoteを使ったGoogle Slides変換ガイド

MacユーザーがPDFからGoogle Slidesに変換する最良の方法です。

## なぜKeynoteが良いのか？

- **無料**: Macに標準でインストール済み
- **高品質**: PDFのインポートがスムーズ
- **編集可能**: テキストや画像を編集できる
- **互換性**: Google SlidesがKeynoteファイルをネイティブサポート
- **PowerPoint不要**: 追加のソフトウェアが不要

## ステップバイステップガイド

### ステップ1: PDFを生成

```bash
cd presentation
npm install
npm run export:pdf
```

`cb_presentation.pdf` が生成されます。

### ステップ2: KeynoteでPDFを開く

1. **Keynoteを起動**
   - Launchpadから「Keynote」を検索
   - または Applications フォルダから起動

2. **PDFをインポート**
   - 「ファイル」→「開く」（またはCmd+O）
   - `cb_presentation.pdf` を選択
   - 「開く」をクリック

3. **インポート確認**
   - PDFの各ページが個別のスライドとしてインポートされます
   - 25枚のスライドが表示されることを確認

### ステップ3: Keynoteで調整（オプション）

#### スライドサイズの調整

1. 右側のパネルで「書類」タブを選択
2. 「スライドのサイズ」を選択
   - 「ワイドスクリーン (16:9)」を推奨
   - 「標準 (4:3)」も選択可能

#### 編集可能にする（必要に応じて）

PDFからインポートしたスライドは画像として扱われますが、以下が可能です：

- **テキストボックスの追加**:
  1. ツールバーの「T」アイコンをクリック
  2. スライド上でテキストを追加

- **画像の追加**:
  1. 「挿入」→「選択」
  2. 画像ファイルを選択

- **図形の追加**:
  1. ツールバーの図形アイコンをクリック
  2. 四角形、円、矢印などを追加

- **アニメーションの追加**:
  1. 右側のパネルで「アニメーション」タブ
  2. 「エフェクトを追加」をクリック

### ステップ4: Google Slidesで開く

#### 方法A: Keynote形式で書き出し（推奨）

1. **Keynoteファイルとして保存**
   ```
   ファイル → 書き出す → Keynote...
   ```
   - ファイル名: `cb_presentation.key`
   - 保存先を選択
   - 「書き出す」をクリック

2. **Google Driveにアップロード**
   - https://drive.google.com を開く
   - 「新規」→「ファイルのアップロード」
   - `cb_presentation.key` を選択

3. **Google Slidesで開く**
   - アップロードしたファイルをダブルクリック
   - 「Googleスライドで開く」をクリック
   - 自動的に変換されます

#### 方法B: PowerPoint形式で書き出し

1. **PowerPoint形式で書き出し**
   ```
   ファイル → 書き出す → PowerPoint...
   ```
   - ファイル名: `cb_presentation.pptx`
   - 「書き出す」をクリック

2. **Google Driveにアップロード**
   - .pptxファイルをアップロード

3. **Google Slidesで開く**
   - ファイルをダブルクリック
   - 「Googleスライドで開く」をクリック

## 品質比較

| 形式 | 編集可能性 | レイアウト保持 | 互換性 |
|------|-----------|--------------|--------|
| .key → Google Slides | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| .pptx → Google Slides | ⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| PDF → Google Slides | ⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ |

**推奨**: .key形式が最も良い結果を提供します。

## トラブルシューティング

### Keynoteが見つからない

**確認方法:**
```bash
open -a Keynote
```

**インストールされていない場合:**
1. App Storeを開く
2. "Keynote"を検索
3. 「入手」または「ダウンロード」をクリック（無料）

### PDFがうまくインポートできない

**対処法:**
1. PDFを再生成:
   ```bash
   npm run export:pdf
   ```

2. Keynoteを最新版にアップデート:
   - App Store → 「アップデート」タブ

3. macOSをアップデート（必要に応じて）

### Google Slidesで文字化けする

**原因**: フォントの互換性

**対処法:**
1. Keynoteでフォントを標準的なものに変更:
   - Arial
   - Helvetica
   - Times New Roman
   - Courier

2. または、画像エクスポート方式を使用:
   ```bash
   npm run export:images
   ```

### レイアウトが崩れる

**対処法1**: .key形式を使う
- PowerPoint形式よりも互換性が高い

**対処法2**: 画像として保持
- Google Slidesでインポート後、編集せずに使用

**対処法3**: 画像エクスポート方式
- 完全な見た目を保持したい場合

## ベストプラクティス

### 見た目重視の場合

```bash
# 画像としてエクスポート
npm run export:images

# Google Slidesで各画像をスライドに配置
```

### 編集重視の場合

```bash
# PDFを生成
npm run export:pdf

# Keynoteでインポート → 編集 → .key形式で書き出し → Google Slides
```

### 両方のメリットを活かす

1. **ベーススライド**: 画像エクスポートを使用
2. **編集可能なスライド**: Keynote経由で作成
3. **2つのバージョンを管理**:
   - プレゼン用（画像版・高品質）
   - 編集用（Keynote/Google Slides版）

## 自動化スクリプト（オプション）

Keynoteファイルの生成を自動化することも可能です：

```bash
# macOS専用の自動化スクリプト（将来実装予定）
npm run export:keynote
```

現時点では、Keynote AppleScriptを使った自動化は可能ですが、
手動での変換が最も確実な方法です。

## まとめ

MacユーザーなレKeynote経由が**最適**：

1. ✅ 無料
2. ✅ 高品質
3. ✅ 編集可能
4. ✅ Google Slidesとの互換性が高い
5. ✅ PowerPoint不要

```
PDF → Keynote → .key → Google Slides
```

この流れで、高品質で編集可能なプレゼンテーションがGoogle Slidesで手に入ります。
