# 文字列補間テストスイート

## 概要

Cb言語の文字列補間機能（Python/C#風`{式}`構文 + フォーマット指定子）のテストケース集です。変数や式を直接`{}`内に記述するシンプルで直感的な記法を採用しています。

## 基本構文

```cb
// 変数の直接埋め込み
string name = "World";
println("Hello, {name}!");  // Hello, World!

// 式の評価
int a = 10, b = 20;
println("Sum: {a + b}");    // Sum: 30

// フォーマット指定
double pi = 3.14159;
println("Pi: {pi:.2}");     // Pi: 3.14
```

## テストファイル

### 1. basic_interpolation.cb
基本的な文字列補間機能のテスト

**テスト内容**:
- 単一変数の補間
- 複数変数の補間
- 各種データ型（int, double, string, char, bool）
- 混在型の補間

**実行方法**:
```bash
./main tests/cases/string_interpolation/basic_interpolation.cb
```

---

### 2. format_specifiers.cb
フォーマット指定子のテスト

**テスト内容**:
- 整数フォーマット: `{:x}`, `{:X}`, `{:o}`, `{:b}`
- 幅指定: `{:5}`, `{:05}`, `{:<5}`, `{:^5}`, `{:>5}`
- 浮動小数点フォーマット: `{:.2}`, `{:e}`, `{:E}`
- 文字列フォーマット: `{:10}`, `{:.3}`

**実行方法**:
```bash
./main tests/cases/string_interpolation/format_specifiers.cb
```

---

### 3. expression_evaluation.cb
式の評価テスト

**テスト内容**:
- 算術式: `{a + b}`, `{a * b}`
- 関数呼び出し: `{square(5)}`
- メンバーアクセス: `{p.name}`
- 配列要素アクセス: `{nums[2]}`
- 条件式（三項演算子）: `{score >= 80 ? "A" : "B"}`
- ビット演算: `{x << 2}`
- 式とフォーマットの組み合わせ: `{square(5):04}`

**実行方法**:
```bash
./main tests/cases/string_interpolation/expression_evaluation.cb
```

---

### 4. advanced_features.cb
高度な機能のテスト

**テスト内容**:
- エスケープシーケンス: `{{}}`, `{{x}}`
- カスタムフィル文字: `{num:*>5}`, `{num:-<5}`, `{num:.^7}`
- プレフィックス付き16進数: `{num:#x}`, `{num:#X}`
- 符号表示: `{num:+}`
- 同じ変数の再利用: `{val} * {val} = {val * val}`

**実行方法**:
```bash
./main tests/cases/string_interpolation/advanced_features.cb
```

---

### 5. practical_examples.cb
実用的な使用例

**テスト内容**:
- ログ出力形式: `[{level}] {file}:{line} - message`
- テーブル表示: ヘッダー、罫線、データ行（変数を直接使用）
- プログレスバー: `[{progress}/{total}] {percent:.1}%`
- 16進ダンプ: `{offset:#06x}`, `{bytes[i]:02x}`
- 座標表示: `({x:.2}, {y:.2})`
- デバッグ出力: `{debug_x:?}`

**実行方法**:
```bash
./main tests/cases/string_interpolation/practical_examples.cb
```

---

## 全テスト実行

```bash
# すべてのテストを実行
for file in tests/cases/string_interpolation/*.cb; do
    echo "Running $file..."
    ./main "$file"
    echo ""
done
```

---

## フォーマット指定子リファレンス

### 基本構文: `{式:format}`

変数や式を直接`{}`内に記述し、`:`の後にフォーマット指定を追加します。

### 整数型

| 指定子 | 説明 | 例 | 結果 |
|--------|------|-----|------|
| `{num}` | デフォルト（10進数） | `println("{num}", num=255)` | `255` |
| `{num:x}` | 16進数（小文字） | `println("{num:x}", num=255)` | `ff` |
| `{num:X}` | 16進数（大文字） | `println("{num:X}", num=255)` | `FF` |
| `{num:o}` | 8進数 | `println("{num:o}", num=255)` | `377` |
| `{num:b}` | 2進数 | `println("{num:b}", num=255)` | `11111111` |
| `{num:5}` | 幅5（右寄せ） | `println("{num:5}", num=42)` | `   42` |
| `{num:05}` | ゼロパディング | `println("{num:05}", num=42)` | `00042` |
| `{num:<5}` | 左寄せ | `println("{num:<5}", num=42)` | `42   ` |
| `{num:^5}` | 中央寄せ | `println("{num:^5}", num=42)` | ` 42  ` |
| `{num:#x}` | プレフィックス付き16進 | `println("{num:#x}", num=255)` | `0xff` |

### 浮動小数点型

| 指定子 | 説明 | 例 | 結果 |
|--------|------|-----|------|
| `{pi}` | デフォルト | `println("{pi}", pi=3.14159)` | `3.141590` |
| `{pi:.2}` | 小数点以下2桁 | `println("{pi:.2}", pi=3.14159)` | `3.14` |
| `{pi:8.2}` | 幅8、精度2 | `println("{pi:8.2}", pi=3.14)` | `    3.14` |
| `{pi:e}` | 科学的記法（小文字） | `println("{pi:e}", pi=3.14)` | `3.14e+00` |
| `{pi:E}` | 科学的記法（大文字） | `println("{pi:E}", pi=3.14)` | `3.14E+00` |
| `{pi:.2e}` | 科学的記法（精度2） | `println("{pi:.2e}", pi=3.14)` | `3.14e+00` |

### 文字列型

| 指定子 | 説明 | 例 | 結果 |
|--------|------|-----|------|
| `{name}` | デフォルト | `println("{name}", name="text")` | `text` |
| `{name:10}` | 幅10（右寄せ） | `println("{name:10}", name="text")` | `      text` |
| `{name:<10}` | 左寄せ | `println("{name:<10}", name="text")` | `text      ` |
| `{name:^10}` | 中央寄せ | `println("{name:^10}", name="text")` | `   text   ` |
| `{name:.3}` | 最大3文字 | `println("{name:.3}", name="hello")` | `hel` |

### カスタムフィル

| 指定子 | 説明 | 例 | 結果 |
|--------|------|-----|------|
| `{num:*>5}` | アスタリスクで右寄せ | `println("{num:*>5}", num=42)` | `***42` |
| `{num:-<5}` | ハイフンで左寄せ | `println("{num:-<5}", num=42)` | `42---` |
| `{num:.^7}` | ドットで中央寄せ | `println("{num:.^7}", num=42)` | `..42...` |

---

## 式の埋め込み

Cb言語の文字列補間では、変数だけでなく任意の式を直接埋め込むことができます。

| 種類 | 例 | 説明 |
|------|-----|------|
| 変数 | `{name}` | 変数を直接参照 |
| 算術式 | `{a + b}` | 式を評価して埋め込み |
| 関数呼び出し | `{square(5)}` | 関数の戻り値を埋め込み |
| メンバーアクセス | `{person.name}` | 構造体メンバーにアクセス |
| 配列要素 | `{nums[2]}` | 配列要素を参照 |
| 条件式 | `{x > 0 ? "+" : "-"}` | 三項演算子の結果 |
| 同じ変数の再利用 | `{x} * {x} = {x * x}` | 1つの変数を複数回使用 |

---

## エスケープ

| 記述 | 結果 | 説明 |
|------|------|------|
| `{{` | `{` | 左中括弧のリテラル |
| `}}` | `}` | 右中括弧のリテラル |
| `{{}}` | `{}` | 空の中括弧 |
| `{{x}}` | `{x}` | 変数名を含むリテラル |

---

## 期待される動作

### コンパイル時チェック
- [ ] 未定義変数の参照でエラー
- [ ] 無効なフォーマット指定子の場合、エラー
- [ ] 構文エラー（閉じ括弧なし等）

### ランタイム動作
- [ ] 型に応じた適切な変換
- [ ] フォーマット指定の正確な適用
- [ ] 効率的な文字列構築（余分なコピー回避）
- [ ] 式の正しい評価

---

## 実装ステータス

- [x] 設計ドキュメント作成
- [x] テストケース作成
- [ ] Lexer拡張
- [ ] Parser拡張
- [ ] AST拡張
- [ ] 評価器実装
- [ ] フォーマッタ実装
- [ ] 最適化
- [ ] すべてのテストが成功

---

## 参考

- 設計ドキュメント: `docs/features/string_interpolation.md`
- 実装計画: `docs/todo/string_interpolation_implementation_plan.md`
