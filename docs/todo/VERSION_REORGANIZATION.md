# バージョン番号再編成ドキュメント

**作成日**: 2025年11月14日  
**ステータス**: ✅ 完了

---

## 🎯 再編成の背景

v0.13.0のリリース後、次期バージョン（v0.14.0）の計画を見直した結果、以下の問題が明らかになりました：

### 問題点

1. **v0.14.0の内容がパッチレベル**: テストカバレッジ改善とバグ修正が主体
2. **セマンティックバージョニング違反**: マイナーバージョンアップ（v0.14.0）なのに新機能なし
3. **バージョン番号のインフレ**: 不必要にマイナーバージョンが上がる

### 解決策

**v0.14.0 → v0.13.1 にパッチバージョンとして変更**し、以降のバージョンを1つずつ繰り上げる。

---

## 📋 変更内容

### Before（旧計画）

```
v0.13.0: FFI/プリプロセッサ/マクロ (Released)
v0.14.0: テストカバレッジ向上、バグ修正
v0.15.0: Generic配列サポート
v0.16.0: IR基盤実装
v0.17.0: 複数バックエンド対応
v0.18.0: 標準ライブラリ完全化
```

### After（新計画）

```
v0.13.0: FFI/プリプロセッサ/マクロ (Released)
v0.13.1: テストカバレッジ向上、バグ修正 (パッチ)
v0.14.0: Generic配列サポート (マイナー)
v0.15.0: IR基盤実装 + 複数バックエンド対応 (マイナー)
v0.16.0: 標準ライブラリ完全化 (マイナー)
v0.17.0: パッケージエコシステム (マイナー)
```

---

## 🔄 実施した変更

### 1. ディレクトリのリネーム

```bash
docs/todo/v0.14.0/ → docs/todo/v0.13.1/
docs/todo/v0.15.0/ → docs/todo/v0.14.0/
docs/todo/v0.16.0/ → docs/todo/v0.15.0/
docs/todo/v0.17.0/ → docs/todo/v0.16.0/
docs/todo/v0.18.0/ → docs/todo/v0.17.0/
```

### 2. ファイルのリネーム

**v0.13.1/**:
- `v0.14.0_implementation_plan.md` → `v0.13.1_implementation_plan.md`
- `v0.14.0_generic_array_support.md` → `v0.13.1_generic_array_support.md`
- `v0.14.0_untested_behaviors.md` → `v0.13.1_untested_behaviors.md`

**v0.14.0/**:
- `v0.15.0_implementation_plan.md` → `v0.14.0_implementation_plan.md`
- `v0.15.0_generic_array_support.md` → `v0.14.0_generic_array_support.md`
- `v0.15.0_untested_behaviors.md` → `v0.14.0_untested_behaviors.md`

**v0.15.0/**:
- `v0.16.0_revised_roadmap.md` → `v0.15.0_revised_roadmap.md`

**v0.16.0/**:
- `v0.17.0_stdlib_library_complete.md` → `v0.16.0_stdlib_library_complete.md`

### 3. ドキュメント内容の更新

- `v0.13.1_implementation_plan.md`: バージョン番号を v0.13.1 に変更
- `v0.14.0_implementation_plan.md`: Generic配列サポート内容に更新
- `ROADMAP_v0.14-v0.18_REVISED.md` → `ROADMAP_v0.13.1-v0.17.0_REVISED.md`
- `ROADMAP_v0.16-v0.18_SUMMARY.md` → `ROADMAP_v0.15.0-v0.17.0_SUMMARY.md`

---

## 📊 新しいバージョニング戦略

### セマンティックバージョニングの遵守

```
v0.MINOR.PATCH
```

- **MINOR**: 新機能追加、破壊的変更なし
- **PATCH**: バグ修正、テスト追加、ドキュメント改善

### 各バージョンの分類

| バージョン | 種別 | 内容 | 理由 |
|-----------|------|------|------|
| v0.13.1 | PATCH | テストカバレッジ、バグ修正 | 既存機能の改善 |
| v0.14.0 | MINOR | Generic配列サポート | 新機能追加 |
| v0.15.0 | MINOR | IR基盤、バックエンド対応 | 新機能追加 |
| v0.16.0 | MINOR | 標準ライブラリ完全化 | 新機能追加 |
| v0.17.0 | MINOR | パッケージエコシステム | 新機能追加 |

---

## ✅ 更新されたファイル一覧

### ディレクトリ構造

```
docs/todo/
├── v0.12.1/
├── v0.13.1/                    # ← v0.14.0から改名
│   ├── v0.13.1_implementation_plan.md
│   ├── v0.13.1_generic_array_support.md
│   └── v0.13.1_untested_behaviors.md
├── v0.14.0/                    # ← v0.15.0から改名
│   ├── README.md
│   ├── v0.14.0_implementation_plan.md
│   ├── v0.14.0_generic_array_support.md
│   └── v0.14.0_untested_behaviors.md
├── v0.15.0/                    # ← v0.16.0から改名
│   ├── detailed_design.md
│   ├── implementation_roadmap.md
│   ├── ir_implementation_plan.md
│   ├── v0.15.0_revised_roadmap.md
│   └── (その他8ファイル)
├── v0.16.0/                    # ← v0.17.0から改名
│   ├── README.md
│   ├── stdlib_design.md
│   └── v0.16.0_stdlib_library_complete.md
├── v0.17.0/                    # ← v0.18.0から改名
│   └── README.md
├── ROADMAP_v0.13.1-v0.17.0_REVISED.md  # ← 改名 & 更新
├── ROADMAP_v0.15.0-v0.17.0_SUMMARY.md  # ← 改名
└── VERSION_REORGANIZATION.md            # ← 新規作成
```

### 主要な更新ファイル

1. ✅ `docs/todo/v0.13.1/v0.13.1_implementation_plan.md`
2. ✅ `docs/todo/v0.14.0/v0.14.0_implementation_plan.md`
3. ✅ `docs/todo/ROADMAP_v0.13.1-v0.17.0_REVISED.md`
4. ✅ `docs/todo/ROADMAP_v0.15.0-v0.17.0_SUMMARY.md`
5. ✅ `docs/todo/VERSION_REORGANIZATION.md` (このファイル)

---

## 🎯 今後のアクション

### 即座に実施

- [x] ディレクトリリネーム完了
- [x] ファイルリネーム完了
- [x] ドキュメント内容更新完了
- [x] ROADMAPドキュメント更新完了

### v0.13.1開発開始時

- [ ] `docs/todo/v0.13.1/` のタスクを実行
- [ ] テストカバレッジを67% → 100%に向上
- [ ] Async機能のバグ修正
- [ ] ドキュメント整備

### v0.14.0以降

- [ ] Generic配列サポート実装
- [ ] IR基盤実装
- [ ] 複数バックエンド対応
- [ ] 標準ライブラリ完全化
- [ ] パッケージエコシステム

---

## 📈 期待される効果

### 1. セマンティックバージョニングの遵守

✅ マイナーバージョン: 新機能追加  
✅ パッチバージョン: バグ修正・改善

### 2. バージョン番号の合理化

- 不必要なマイナーバージョンアップを防止
- ユーザーにとって明確なバージョン履歴

### 3. 開発計画の明確化

- 各バージョンの目的が明確
- 実装優先度が明確

---

## 📝 メモ

### 変更の妥当性

**v0.13.1（旧v0.14.0）の内容**:
- テストカバレッジ改善: 既存機能のテスト追加
- Asyncバグ修正: 既存機能の修正
- パターンマッチング強化: 既存機能の改善

→ **新機能追加なし** = パッチバージョンが妥当

**v0.14.0（旧v0.15.0）の内容**:
- Generic配列サポート: 新機能追加
- IR基盤実装: 新機能追加

→ **新機能追加あり** = マイナーバージョンが妥当

---

**結論**: バージョン番号の再編成は、セマンティックバージョニングの原則に基づいた合理的な判断です。

---

**作成者**: Cb言語開発チーム  
**承認日**: 2025年11月14日
