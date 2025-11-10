# Cbインタプリタ プレゼンテーション

## 概要
「バイブコーディングで作る自作言語Cbインタプリタ！」のプレゼンテーションスライドです。

- **形式**: reveal.js（HTML）
- **時間**: 約20分
- **スライド数**: 25枚

## 🚀 クイックスタート

### ブラウザで直接プレゼン

HTMLファイルをダブルクリックするか、ブラウザで開くだけで使用できます：

```bash
# Macの場合
open cb_interpreter_presentation.html

# Linuxの場合
xdg-open cb_interpreter_presentation.html

# Windowsの場合
start cb_interpreter_presentation.html
```

### PDFとして出力（1分で完了）✨

```bash
# インタラクティブスクリプトを実行
./export.sh

# または直接npmコマンドで
npm install
npm run export:pdf
```

**詳細**: [`QUICK_START.md`](QUICK_START.md) をご覧ください

### 操作方法

- **→** または **Space**: 次のスライドへ
- **←**: 前のスライドへ
- **Esc**: スライド一覧表示
- **F**: フルスクリーンモード
- **S**: スピーカーノート表示（発表者用）
- **?**: ヘルプ表示

## PDF変換方法

### 方法1: 自動スクリプト（最も簡単・推奨）✨

```bash
# 依存関係のインストール（初回のみ）
npm install

# 高品質PDFを生成
npm run export:pdf
```

生成されたPDFファイル: `cb_presentation.pdf`

### 方法2: ブラウザの印刷機能

1. **Google ChromeまたはMicrosoft Edgeでスライドを開く**
2. **URLの末尾に `?print-pdf` を追加**
   ```
   file:///path/to/cb_interpreter_presentation.html?print-pdf
   ```
3. **Ctrl+P（またはCmd+P）で印刷ダイアログを開く**
4. **送信先を「PDFに保存」に変更**
5. **設定を以下のように調整:**
   - レイアウト: 横向き
   - 余白: なし
   - 背景のグラフィック: オン
6. **保存**

### 方法3: decktape（コマンドライン）

```bash
# decktapeのインストール
npm install -g decktape

# PDF変換
decktape reveal cb_interpreter_presentation.html cb_presentation.pdf

# サイズ指定
decktape reveal -s 1920x1080 cb_interpreter_presentation.html cb_presentation.pdf
```

## Google Slidesへの変換

reveal.jsのHTMLスライドを直接Google Slidesに変換することはできませんが、以下の方法があります：

### 方法1: 画像ファイルとしてエクスポート（最高品質）✨✨

各スライドを個別の高解像度PNG画像として保存し、Google Slidesにインポートします。

```bash
# 依存関係のインストール（初回のみ）
npm install

# 全スライドを画像として保存
npm run export:images
```

生成された画像: `slides/slide_01.png`, `slide_02.png`, ... `slide_25.png`

**Google Slidesにインポート:**
1. Google Slidesで新規プレゼンテーションを作成
2. 各スライドで:
   - 「挿入」→「画像」→「パソコンからアップロード」
   - 対応するPNGファイルを選択
   - 画像をスライド全体に拡大

**メリット**:
- 最高品質の見た目を維持
- フォントやレイアウトが完全に保持される
- 編集は画像の上に新しい要素を追加する形

### 方法2: Keynote経由（Mac推奨・編集可能性が高い）✨✨

PDFをKeynoteでインポートし、Google Slidesで開きます。

```bash
# 1. PDFを生成
npm run export:pdf
```

**KeynoteでPDFをインポート:**
1. **Keynoteを起動**
2. **「ファイル」→「開く」**
3. **cb_presentation.pdfを選択**
4. PDFの各ページが個別のスライドとしてインポートされます

**Keynoteファイルの調整（オプション）:**
- スライドサイズを調整：「書類」→「スライドのサイズ」
- テキストの編集や画像の追加が可能
- アニメーションの追加も可能

**Google Slidesで開く:**

**方法A: Keynoteファイルを直接アップロード（推奨）**
1. Keynoteで「ファイル」→「書き出す」→「Keynote」を選択
2. .keyファイルを保存
3. Google Driveにアップロード
4. ダブルクリックで開く
5. 「Googleスライドで開く」をクリック

**方法B: PowerPoint形式で書き出し**
1. Keynoteで「ファイル」→「書き出す」→「PowerPoint」
2. .pptxファイルを保存
3. Google Driveにアップロード
4. 「Googleスライドで開く」をクリック

**メリット**:
- Macユーザーなら無料（標準アプリ）
- PDFのインポートが簡単
- レイアウトがよく保持される
- Google SlidesがKeynoteをネイティブサポート
- テキストや画像の編集が可能

### 方法3: PowerPoint経由（Windows推奨）

PDFをPowerPointに変換してから、Google Slidesで開きます。

```bash
# 1. PDFを生成
npm run export:pdf
```

**PDFをPowerPointに変換:**
- **Adobe Acrobat Pro**: 「ファイル」→「書き出し」→「Microsoft PowerPoint」
- **オンラインツール**:
  - https://smallpdf.com/pdf-to-ppt
  - https://www.ilovepdf.com/pdf_to_powerpoint
  - https://www.adobe.com/acrobat/online/pdf-to-ppt.html

**PowerPointをGoogle Slidesで開く:**
1. .pptxファイルをGoogleドライブにアップロード
2. ダブルクリックでGoogle Slidesで開く
3. 「Googleスライドで開く」を選択

**メリット**:
- Windows標準のプレゼンテーションソフト
- テキストが編集可能
- レイアウトがある程度保持される

### 方法4: PDFを直接インポート（最も簡単）

1. **PDFに変換**（上記の方法1を使用）
2. **Google Slidesを開く** (https://slides.google.com)
3. **「ファイル」→「インポート」→「アップロード」**
4. **cb_presentation.pdfを選択**
5. **「すべてインポート」をクリック**

**注意**:
- 各ページが1枚のスライドになります
- テキストは画像として扱われ、編集不可
- フォントやレイアウトは保持されますが、編集できません

### 方法5: 手動で再作成（完全な編集可能性）

Google Slidesで完全な編集可能性が必要な場合：

1. HTMLスライドを参考資料として表示
2. Google Slidesで一から作成
3. テキスト、画像、レイアウトを手動で再現

**メリット**:
- 完全な編集可能性
- Googleドライブでの共有が簡単
- コメント機能やリアルタイム共同編集

**デメリット**:
- 時間がかかる
- 手作業が必要

### 推奨する変換フロー

**最高品質を求める場合:**
```
reveal.js → 画像エクスポート → Google Slidesに画像をインポート
```

**編集可能性を求める場合（Mac）:**
```
reveal.js → PDF → Keynote → .keyファイル → Google Slides
```

**編集可能性を求める場合（Windows）:**
```
reveal.js → PDF → PowerPoint → Google Slides
```

**最も簡単な方法:**
```
reveal.js → PDF → Google Slidesに直接インポート
```

## PowerPointへの変換

### 方法1: PDFをインポート

1. **PDFに変換（上記参照）**
2. **PowerPointを開く**
3. **「挿入」→「画像」→「画像をファイルから」**
4. **PDFファイルを選択**
5. **各ページを個別のスライドとしてインポート**

### 方法2: Adobe Acrobatを使用

Adobe Acrobat Pro DCをお持ちの場合：

1. **PDFに変換**
2. **Adobe Acrobatで開く**
3. **「ファイル」→「書き出し」→「Microsoft PowerPoint」**
4. **変換オプションを選択して保存**

## スライドのカスタマイズ

### テーマの変更

HTMLファイルの以下の行を編集：

```html
<link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/reveal.js@4.5.0/dist/theme/black.css">
```

利用可能なテーマ：
- black（デフォルト）
- white
- league
- beige
- sky
- night
- serif
- simple
- solarized

### トランジションの変更

JavaScriptの設定部分を編集：

```javascript
Reveal.initialize({
    transition: 'slide', // none/fade/slide/convex/concave/zoom
    // ...
});
```

## トラブルシューティング

### スライドがうまく表示されない

- **インターネット接続を確認**: reveal.jsのライブラリをCDNから読み込んでいます
- **別のブラウザで試す**: Chrome、Firefox、Edgeを推奨
- **JavaScriptが有効か確認**

### PDFの画質が悪い

- **decktapeを使用する**: 方法2が最も高品質
- **ブラウザの倍率を150%にしてから印刷**
- **高解像度設定を使用**:
  ```bash
  decktape reveal -s 2560x1440 cb_interpreter_presentation.html cb_presentation.pdf
  ```

### Google Slidesでフォントがずれる

- **PDFインポート時の制限**: 完全な再現は困難
- **手動で調整**: インポート後に個別に修正
- **Webフォントを使用**: Google Fontsを利用

## スライド内容の概要

1. タイトル
2. 動機
3. Cbとは
4. 設計思想
5. 型システム
6. ジェネリクス
7. パターンマッチング
8. RAII/デストラクタ
9. Async/Await
10. アーキテクチャ
11. パーサー実装
12. 評価器実装
13. 変数管理
14. 非同期処理
15. テスト戦略
16. コードデモ
17. バイブコーディング
18. 開発の学び
19. バージョン履歴
20. 今後の展望
21. 実装統計
22. リファクタリング
23. ドキュメント
24. まとめ
25. Q&A

## ライセンス

このプレゼンテーションは、Cb言語プロジェクトの一部です。

## 連絡先

- GitHub: https://github.com/shadowlink0122/Cb
- プロジェクト: Cb (シーフラット) プログラミング言語
