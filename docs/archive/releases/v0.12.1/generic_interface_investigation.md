# ジェネリックinterface + 複合型の完全実装

## 現在の状態（2025-11-10）

### 実装進捗
- ✅ Phase 1: データ構造の拡張完了
  - `ParsedTypeInfo`に`generic_original_type`フィールド追加
  - `InterfaceMember`に`return_type_name_original`フィールド追加
- ⚠️ Phase 2: 部分的に実装（バグ有り）
  - `parseType()`で`generic_original_type`を設定
  - interface定義時に元の型名を保存
  - **問題**: メモリエラー（std::bad_alloc, segfault）が発生

### 発見された問題

1. **メモリ管理の問題**
   - 変更後、既存のinterfaceテストでメモリエラーが発生
   - 原因: `ParsedTypeInfo`の初期化または`getLastParsedTypeInfo()`の使用法に問題がある可能性

2. **asyncラッピング後の型名の不一致**
   - `async Result<T, string>`が`Future<Result<T, string>>`になる
   - しかし、`generic_original_type`には`Result<T, string>`のみが保存される
   - 比較時にミスマッチが発生する

### 必要な追加作業

1. **メモリエラーの修正**
   - `ParsedTypeInfo`のコピーコンストラクタ/代入演算子の確認
   - `getLastParsedTypeInfo()`の返す値が有効かどうかの確認
   - スタック/ヒープの使用を確認

2. **asyncラッピング後の型名の保持**
   - `return_type_name_original`を保存する前に、asyncラッピングを考慮
   - または、interface側でも同様の処理を行う

3. **安全な実装方法の検討**
   - 段階的な実装: まず非ジェネリックで動作確認
   - メモリ安全性の確保
   - 既存機能への影響を最小限に

## 代替アプローチ（推奨）

### より安全な実装: 文字列置換の改善のみ

現在のデータ構造を変更せず、型パラメータ置換ロジックのみを改善する：

```cpp
std::string replaceTypeParameterInComplexType(
    const std::string& type_str,
    const std::string& type_param,
    const std::string& concrete_type
) {
    // Result_T_string を Result_int_string に置換
    // パターン: _T_ または _T$ (終端)
    std::string result = type_str;
    std::string pattern1 = "_" + type_param + "_";
    std::string replacement1 = "_" + concrete_type + "_";
    
    size_t pos = 0;
    while ((pos = result.find(pattern1, pos)) != std::string::npos) {
        result.replace(pos, pattern1.length(), replacement1);
        pos += replacement1.length();
    }
    
    // 末尾のケース: Result_T$
    std::string pattern2 = "_" + type_param;
    if (result.length() >= pattern2.length() &&
        result.substr(result.length() - pattern2.length()) == pattern2) {
        result = result.substr(0, result.length() - pattern2.length()) +
                 "_" + concrete_type;
    }
    
    return result;
}
```

この方法は：
- 既存のコードを壊さない
- メモリ安全
- 複雑な型（`Result<T, E>`）にも対応
- 実装が単純

## 次のステップ

1. 現在の変更をロールバック
2. より安全なアプローチで再実装
3. 段階的にテスト
4. 各段階でメモリリークチェック

## 参考: 成功した機能

以下は既に動作している：
- シンプルなジェネリックinterface（`interface Container<T>`）
- 非ジェネリックでのasync interface
- ジェネリック構造体
- async関数の自動Futureラッピング

## タイムスタンプ
- 開始: 2025-11-10 08:55
- Phase 1完了: 2025-11-10 09:30
- メモリエラー発見: 2025-11-10 09:45

