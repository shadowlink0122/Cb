# Pattern Matching Tests

パターンマッチング機能の包括的なテストスイート

## テストファイル一覧

### 基本機能テスト

1. **test_minimal.cb**
   - 最小限の動作確認
   - Enum変数の作成のみ

2. **test_match_option_basic.cb**
   - Option<T>型の基本的なパターンマッチング
   - Some/Noneの両パターンテスト

3. **test_match_result_basic.cb**
   - Result<T, E>型の基本的なパターンマッチング
   - Ok/Errの両パターンテスト
   - 関数からの返り値テスト

### 高度な機能テスト

4. **test_match_wildcard.cb**
   - ワイルドカードパターン（_）のテスト
   - 複数バリアントの一部のみマッチ

5. **test_match_nested.cb**
   - ネストされたmatch文のテスト
   - Result内でOptionをチェック

6. **test_match_multiple_arms.cb**
   - 多数のmatch armのテスト（5つのバリアント）
   - 網羅的なパターンマッチング

7. **test_match_without_binding.cb**
   - 変数束縛なしのパターンマッチング
   - アンダースコア（_）による値の無視

8. **test_match_return_value.cb**
   - 関数の返り値に対する直接的なパターンマッチング
   - エラーハンドリングの実例

9. **test_match_with_switch.cb**
   - switch文とmatch文の併用テスト
   - 整数条件分岐→Enum処理の組み合わせ

## テスト実行方法

### 個別テスト
```bash
./main tests/cases/pattern_matching/test_match_option_basic.cb
./main tests/cases/pattern_matching/test_match_result_basic.cb
```

### 統合テスト
```bash
make test
```

## 期待される出力

各テストは以下の形式で出力します：
```
=== Test N: [テスト名] ===
[テスト内容]
Test N: PASSED
```

## カバー範囲

✅ Option<T>型のパターンマッチング
✅ Result<T, E>型のパターンマッチング
✅ ワイルドカードパターン（_）
✅ 変数束縛あり/なし
✅ ネストされたmatch文
✅ 複数のmatch arm（網羅性）
✅ 関数返り値への直接マッチング
✅ switch文との併用

## 今後の追加予定

⏳ エラーハンドリングテスト（網羅性チェック失敗等）
⏳ パフォーマンステスト
⏳ 構造体分解（将来機能）
⏳ タプル分解（将来機能）
⏳ 配列パターン（将来機能）
