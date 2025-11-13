# VSCode拡張機能のバージョン管理

VSCode拡張機能のバージョンは、ルートディレクトリの`VERSION`ファイルと自動的に同期されます。

## バージョン管理の仕組み

```
Cb/
├── .cbversion                       # マスターバージョンファイル (例: 0.13.0)
└── vscode-extension/
    ├── package.json                 # 自動更新される
    └── scripts/
        ├── update-version.js        # バージョン更新スクリプト
        └── verify-version.js        # バージョン検証スクリプト
```

## 使用方法

### 1. バージョンの更新

Cbのバージョンを変更する場合：

```bash
# 1. ルートの.cbversionファイルを編集
echo "0.14.0" > VERSION

# 2. VSCode拡張機能のバージョンを自動更新
cd vscode-extension
npm run update-version
```

これにより、`vscode-extension/package.json`のバージョンが自動的に更新されます。

### 2. バージョンの検証

バージョンが一致しているか確認：

```bash
cd vscode-extension
npm run verify-version
```

✅ 成功時:
```
✅ Version check passed: 0.13.0
   VSCode extension version matches .cbversion file
```

❌ 不一致時:
```
❌ Version mismatch detected!
   .cbversion file: 0.14.0
   package.json: 0.13.0

💡 Fix by running: cd vscode-extension && npm run update-version
```

### 3. パッケージング前の自動チェック

VSCode拡張機能をパッケージする際、自動的にバージョンチェックが実行されます：

```bash
cd vscode-extension
npm run package
```

バージョンが一致していない場合、パッケージングは失敗します。

## npm スクリプト

`vscode-extension/package.json`で定義されているスクリプト：

| スクリプト | 説明 |
|-----------|------|
| `npm run update-version` | .cbversionファイルからバージョンを自動更新 |
| `npm run verify-version` | バージョンの一致を検証 |
| `npm run prepackage` | パッケージング前の自動検証 |
| `npm run package` | VSCode拡張機能をパッケージ化 |

## ワークフロー例

### 新しいバージョンをリリースする場合

```bash
# 1. ルートディレクトリで作業
cd /path/to/Cb

# 2. .cbversionファイルを更新
echo "0.14.0" > VERSION

# 3. 変更をコミット
git add VERSION
git commit -m "Bump version to 0.14.0"

# 4. VSCode拡張機能のバージョンを更新
cd vscode-extension
npm run update-version

# 5. 更新されたpackage.jsonをコミット
git add package.json
git commit -m "Update VSCode extension version to 0.14.0"

# 6. パッケージング（自動的にバージョンチェック）
npm run package

# 7. タグを作成してプッシュ
git tag v0.14.0
git push origin main --tags
```

## CI/CDへの統合

### GitHubアクションの例

```yaml
name: Build VSCode Extension

on:
  push:
    branches: [ main ]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      
      - name: Setup Node.js
        uses: actions/setup-node@v2
        with:
          node-version: '16'
      
      - name: Verify version consistency
        run: |
          cd vscode-extension
          npm run verify-version
      
      - name: Package extension
        run: |
          cd vscode-extension
          npm install -g vsce
          npm run package
```

## トラブルシューティング

### バージョンが一致しない場合

```bash
cd vscode-extension
npm run update-version
```

### .cbversionファイルが見つからない

ルートディレクトリに`VERSION`ファイルを作成してください：

```bash
echo "0.13.0" > VERSION
```

### バージョンフォーマットエラー

.cbversionファイルは`x.y.z`形式である必要があります（例: `0.13.0`）。

正しい形式：
- ✅ `0.13.0`
- ✅ `1.0.0`
- ✅ `10.5.23`

間違った形式：
- ❌ `0.13` (zが不足)
- ❌ `v0.13.0` (vプレフィックス)
- ❌ `0.13.0-beta` (サフィックス)

## 利点

1. **単一の真実の源**: .cbversionファイルが唯一のバージョン情報源
2. **自動同期**: スクリプトで自動的に同期
3. **エラー防止**: パッケージング前にバージョンチェック
4. **CI/CD対応**: 簡単にCI/CDパイプラインに統合可能
5. **手動編集不要**: package.jsonを直接編集する必要がない

## 注意事項

- ✅ .cbversionファイルを変更したら、必ず`npm run update-version`を実行
- ✅ package.jsonのversionフィールドを直接編集しない
- ✅ コミット前にバージョンの一致を確認
- ✅ パッケージング前に自動チェックが実行される
