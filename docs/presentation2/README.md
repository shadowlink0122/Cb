# Cb Language Presentation

## 概要
バイブコーディングで作る自作言語Cbインタプリタについてのプレゼンテーション資料です。
TypeScriptとReveal.jsを使用して構築されています。

## ディレクトリ構成

```
presentation2/
├── src/                    # ソースコード
│   ├── main.ts            # メインエントリーポイント
│   ├── slide-loader.ts    # スライドローダー
│   └── slides/            # 各スライドのTypeScriptファイル
│       ├── 00_title.ts              # タイトルスライド
│       ├── 01_intro.ts              # 自己紹介
│       ├── 02_cb_overview.ts        # Cbの概要
│       ├── section1_cover.ts        # セクション1表紙
│       ├── section2_cover.ts        # セクション2表紙
│       ├── section3_cover.ts        # セクション3表紙
│       └── section4_cover.ts        # セクション4表紙
├── styles/                # スタイルシート
│   └── main.css          # メインCSS
├── assets/               # 画像などのアセット
├── dist/                 # ビルド出力
├── index.html           # HTMLエントリーポイント
├── package.json         # npm設定
├── tsconfig.json        # TypeScript設定
└── vite.config.ts       # Vite設定
```

## スライド命名規則

スライドファイルは以下の命名規則に従います：
- `00_title.ts` - 番号_内容の形式
- `section{N}_cover.ts` - セクション表紙
- `section{N}_{content}.ts` - セクション内のコンテンツ

## セットアップ

```bash
# 依存関係のインストール
npm install

# 開発サーバーの起動
npm run dev

# ビルド
npm run build

# プレビュー
npm run preview
```

## 開発ガイド

### 新しいスライドの追加

1. `src/slides/`に新しいTypeScriptファイルを作成
2. スライドのHTMLを返す関数をエクスポート
3. `src/slide-loader.ts`でインポートして登録

例：
```typescript
// src/slides/section1_example.ts
export default function exampleSlide(): string {
    return `
        <section>
            <h2>Example Slide</h2>
            <p>Content here</p>
        </section>
    `;
}
```

### スタイルのカスタマイズ

`styles/main.css`でスタイルを定義しています。
各スライドタイプ（title-slide、intro-slide等）に対応するクラスが用意されています。

## ビルドとデプロイ

```bash
# プロダクションビルド
npm run build
```

ビルドされたファイルは`dist/`ディレクトリに出力されます。

## 技術スタック

- **TypeScript**: 型安全な開発
- **Reveal.js**: プレゼンテーションフレームワーク
- **Vite**: 高速なビルドツール
- **CSS**: カスタムスタイリング