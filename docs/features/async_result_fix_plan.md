# async + Result統合修正プラン

## 問題
await式がint64_tを返すため、Result<T,E>のようなenum情報を保持できない

## 解決策
1. awaitの戻り値をTypedValueに変更
2. 変数宣言時にTypedValueからenum情報を正しく取得
3. EventLoopでのenum情報の完全保持を確認

## 実装手順
1. evaluate_awaitの戻り値をTypedValueに変更
2. dispatcherでTypedValueを処理
3. 変数宣言でTypedValueからenum情報を抽出
