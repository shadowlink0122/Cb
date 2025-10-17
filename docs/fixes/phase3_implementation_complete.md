# Phase 3 Implementation Complete: Token-Based Preprocessor Integration

## 実装日
2024年（タイムスタンプ記録用）

## 概要
トークンベースのプリプロセッサをCbコンパイラのmain.cppに統合し、文字列リテラル内のマクロ展開バグを修正しました。

## 実装内容

### 1. Phase 1: Lexer拡張 ✅
**ファイル**: `src/frontend/recursive_parser/recursive_lexer.h/cpp`

**追加したトークンタイプ**:
- `TOK_HASH` (#)
- `TOK_PREPROCESSOR_DEFINE` (#define)
- `TOK_PREPROCESSOR_UNDEF` (#undef)

**実装詳細**:
- `makePreprocessorDirective()` メソッドを追加
- 行全体を読み取り、ディレクティブトークンとして返す
- `!isAtEnd()`チェックを追加してバッファオーバーランを防止

### 2. Phase 2: TokenPreprocessor実装 ✅
**ファイル**: `src/frontend/preprocessor/token_preprocessor.h/cpp`

**主要機能**:
- トークンストリームからプリプロセッサディレクティブを処理
- `TOK_STRING`タイプのトークンはマクロ展開から保護
- オブジェクトマクロと関数マクロの両方をサポート
- 再帰的マクロ展開をサポート
- エラーハンドリング機能

**テストカバレッジ**: 6つのテストケース全てパス

### 3. Phase 3: main.cpp統合 ✅
**ファイル**: `src/frontend/main.cpp`

**統合戦略**: ハイブリッドアプローチ
1. ソースコードをトークン化
2. プリプロセッサディレクティブの有無を検出
3. ディレクティブがある場合のみTokenPreprocessorを実行
4. トークンを文字列に変換してRecursiveParserに渡す
5. ディレクティブがない場合は元のソースをそのまま使用

**重要な実装**:
```cpp
std::string tokensToString(const std::vector<Token>& tokens) {
    // 文字列リテラルには引用符を追加
    if (token.type == TokenType::TOK_STRING) {
        result += "\"" + token.value + "\"";
    } else {
        result += token.value;
    }
}
```

**理由**: 
- RecursiveParserを変更せずに済むため安全
- プリプロセッサディレクティブがないファイルは元のまま処理（パフォーマンス）
- すべての既存テストとの後方互換性を維持

## テスト結果

### ユニットテスト: 54/54 成功 ✅
- Lexer Preprocessor Tests: 5テスト
- Token Preprocessor Tests: 6テスト
- Backend Tests: 30テスト
- その他Preprocessor Tests: 13テスト

### 統合テスト: 全て成功 ✅
- 既存の2968+テストが全てパス
- プリプロセッサディレクティブなしのファイルは影響なし

### デモプログラム検証 ✅
**`tests/cases/preprocessor/string_literal_protection_demo.cb`**:
```cb
#define PI 3.14159
#define MAX(a, b) ((a) > (b) ? (a) : (b))

int main() {
    println("PI value: %f", PI);  // PIは展開される
    println("The constant PI represents...");  // 文字列内のPIは展開されない ✅
    println("MAX function can be used...");     // 文字列内のMAXは展開されない ✅
    
    int max_val = MAX(10, 20);  // MAX関数マクロは正しく展開される
    println("MAX(%d, %d) = %d", 10, 20, max_val);
}
```

**出力**:
```
PI value: 3.141590
The constant PI represents the ratio of a circle's circumference to its diameter.
MAX function can be used to find the maximum value
MAX(10, 20) = 20
Test passed: String literals are protected from macro expansion!
```

## 問題と解決策

### 問題1: Phase 3初回実装で無限ループ
**原因**: RecursiveParserに直接トークンストリームモードを追加しようとした
**解決**: ロールバックして、ハイブリッドアプローチ（トークン→文字列変換）を採用

### 問題2: 文字列リテラルの引用符が失われる
**原因**: `RecursiveLexer::makeString()`が引用符をトリミング
**解決**: `tokensToString()`で`TOK_STRING`タイプのトークンに引用符を再追加

### 問題3: トークン間のスペーシング
**原因**: トークンを文字列に変換する際の適切なスペーシングルールが必要
**解決**: トークンタイプに基づいて条件付きでスペースを挿入

## 成果

✅ **文字列リテラルバグ修正**: `println("PI = ")` が `println("3.14159 = ")` に展開される問題を解決
✅ **トークンベース処理**: 文字列リテラルは`TOK_STRING`として識別され、マクロ展開から完全に保護される
✅ **後方互換性**: プリプロセッサディレクティブがないファイルには影響なし
✅ **全テスト成功**: 54ユニットテスト + 2968+統合テスト
✅ **関数マクロサポート**: `MAX(a, b)` のような関数マクロも正しく動作
✅ **再帰的展開**: `DOUBLE(DOUBLE(5))` のようなネストされたマクロも動作
✅ **エラーハンドリング**: 不正なディレクティブに対する適切なエラーメッセージ

## ファイル変更サマリー

### 新規ファイル:
- `src/frontend/preprocessor/token_preprocessor.h`
- `src/frontend/preprocessor/token_preprocessor.cpp`
- `tests/unit/frontend/preprocessor/test_token_preprocessor.hpp`
- `tests/unit/frontend/recursive_parser/test_lexer_preprocessor.hpp`
- `tests/cases/preprocessor/string_literal_protection_demo.cb`
- `tests/cases/preprocessor/simple_string_test.cb`
- `docs/fixes/phase3_safe_integration_strategy.md` (設計文書)

### 修正ファイル:
- `src/frontend/recursive_parser/recursive_lexer.h` (トークンタイプ追加)
- `src/frontend/recursive_parser/recursive_lexer.cpp` (makePreprocessorDirective実装)
- `src/frontend/main.cpp` (TokenPreprocessor統合 + tokensToString)
- `tests/unit/main.cpp` (テスト実行関数追加)
- `Makefile` (token_preprocessor.o追加)

## 今後の拡張可能性

Phase 4として以下の機能を追加可能:
1. **条件付きコンパイル**: `#if`, `#ifdef`, `#ifndef`, `#else`, `#endif`
2. **トークン連結**: `##` 演算子
3. **文字列化**: `#` 演算子
4. **ファイルインクルード**: `#include`（既存のimportと統合）
5. **プリ定義マクロ**: `__FILE__`, `__LINE__`, `__DATE__`, `__TIME__`

## まとめ

トークンベースのプリプロセッサ統合が完了し、文字列リテラル内のマクロ展開バグが修正されました。安全なハイブリッドアプローチにより、既存のコードベースに影響を与えず、全てのテストが成功しています。

実装は3つのフェーズに分けて慎重に行われ、各フェーズで徹底的なテストを実施しました。Phase 3では初回実装の失敗から学び、より安全なアプローチを採用することで成功しました。
