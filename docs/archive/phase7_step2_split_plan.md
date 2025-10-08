# Phase 7 Step 2: statement_executor.cpp Split Plan

## 現状分析

### ファイルサイズ
- **statement_executor.cpp**: 3,393行

### 主要メソッドと行数
1. `execute_assignment`: ~774行 (L79-852)
2. `execute_variable_declaration`: ~611行 (L853-1463)
3. `execute_member_assignment`: ~604行 (L2200-2803)
4. `execute_member_array_assignment`: ~392行 (L1808-2199)
5. `execute_array_decl`: ~313行 (L1473-1785)
6. `execute_arrow_assignment`: ~61行 (L2804-2864)
7. その他: 複数の小さなヘルパーメソッド

## 分割戦略

### ファイル構成
```
executors/
├── statement_executor.{cpp,h}         # メインディスパッチャ (100行程度)
├── assignments/
│   ├── simple_assignment.cpp          # 単純代入 (execute_assignment の一部)
│   ├── member_assignment.cpp          # メンバー代入 (execute_member_assignment)
│   ├── array_assignment.cpp           # 配列代入関連
│   ├── union_assignment.cpp           # Union代入 (execute_union_assignment)
│   ├── ternary_assignment.cpp         # 三項演算子代入
│   └── assignment_helpers.h           # 代入関連のヘルパー関数
└── declarations/
    ├── variable_declaration.cpp       # 変数宣言 (execute_variable_declaration)
    ├── array_declaration.cpp          # 配列宣言 (execute_array_decl)
    └── declaration_helpers.h          # 宣言関連のヘルパー関数
```

### フェーズ1: 宣言系の分離
1. `execute_variable_declaration` → declarations/variable_declaration.cpp
2. `execute_array_decl` → declarations/array_declaration.cpp  
3. `execute_multiple_var_decl` → declarations/variable_declaration.cpp

### フェーズ2: 代入系の分離
1. `execute_assignment` (単純代入部分) → assignments/simple_assignment.cpp
2. `execute_member_assignment` → assignments/member_assignment.cpp
3. `execute_member_array_assignment` → assignments/array_assignment.cpp
4. `execute_arrow_assignment` → assignments/member_assignment.cpp
5. `execute_union_assignment` → assignments/union_assignment.cpp
6. `execute_ternary_assignment` → assignments/ternary_assignment.cpp

### フェーズ3: ディスパッチャの最小化
- `statement_executor.cpp`: メインの`execute()`メソッドのみ残す
- 各機能は新しいファイルに委譲

## 実装計画

### Step 2.1: declarations/ の作成
- [ ] declarations/variable_declaration.cpp 作成
- [ ] declarations/array_declaration.cpp 作成
- [ ] statement_executor.hに前方宣言追加
- [ ] ビルド&テスト

### Step 2.2: assignments/ の作成
- [ ] assignments/simple_assignment.cpp 作成
- [ ] assignments/member_assignment.cpp 作成
- [ ] assignments/array_assignment.cpp 作成
- [ ] assignments/union_assignment.cpp 作成
- [ ] assignments/ternary_assignment.cpp 作成
- [ ] ビルド&テスト

### Step 2.3: 統合とテスト
- [ ] Makefile更新
- [ ] 全テスト実行
- [ ] コミット

## 期待される効果

### ファイルサイズ削減
- statement_executor.cpp: 3,393行 → ~150行 (96%削減)
- 平均ファイルサイズ: ~400行

### メンテナンス性向上
- 機能ごとの分離
- テストの容易性
- 並行開発の可能性

## リスク管理

### 高リスク要因
1. メソッド間の依存関係
2. private メンバー変数へのアクセス
3. ヘルパーメソッドの共有

### 対策
1. friend クラス宣言
2. 段階的な分割
3. 各ステップでテスト実行
