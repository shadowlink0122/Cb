# ネストした構造体とポインタアクセスの実装計画

## 目標

1. ネストした構造体メンバアクセス: `box.topLeft.x`
2. ポインタアクセス: `p->member`, `(*p).member`
3. ネストしたポインタ: `p->member->submember`, `(*(*p).member).submember`
4. 混合アクセス: `(*p).member->submember`, `p->member.submember`
5. selfを介したアクセス: impl内で `self.member.submember`, `self->member->submember`
6. チェーン処理: `obj.method1().method2()`, `obj.member1.member2.member3`

## 現状の問題

### 1. ネストした構造体メンバが0を返す

```cb
struct Point { int x; int y; };
struct Box { Point topLeft; };

Box box1;
box1.topLeft.x = 42;
println(box1.topLeft.x);  // 0が出力される（期待値: 42）
```

**原因**: 
- 構造体メンバに構造体型を代入する際、ネストした構造体の初期化が正しく行われていない
- `box1.topLeft` が構造体として認識されていない
- 個別メンバ変数 (`box1.topLeft.x`) が作成されていない

### 2. `->`演算子が未実装

```cb
Box* p = &box1;
p->topLeft.x = 10;  // 構文エラー
```

**原因**:
- `->` 演算子がパーサーで認識されていない、または
- インタプリタで処理されていない

## 実装ステップ

### Phase 1: ネストした構造体メンバの基本サポート ✅ (優先度: 最高)

#### 1.1 構造体変数宣言時のネストメンバ初期化

**場所**: `src/backend/interpreter/managers/variable_manager.cpp`

**変更点**:
- 構造体変数を宣言した際、そのメンバーが構造体型の場合、再帰的に初期化する
- 例: `Box box1` → `box1.topLeft` (Point型) → `box1.topLeft.x`, `box1.topLeft.y`

**実装**:
```cpp
void VariableManager::initialize_nested_struct_members(
    Variable& parent_var, 
    const std::string& parent_path,
    const StructDefinition& struct_def
) {
    for (const auto& member : struct_def.members) {
        std::string member_path = parent_path + "." + member.name;
        
        // メンバーが構造体型の場合
        if (member.type == TYPE_STRUCT && !member.struct_type_name.empty()) {
            // ネストした構造体の定義を取得
            auto nested_def = interpreter_->get_struct_definition(member.struct_type_name);
            if (nested_def) {
                // ネストした構造体変数を作成
                Variable nested_var;
                nested_var.type = TYPE_STRUCT;
                nested_var.is_struct = true;
                nested_var.struct_type_name = member.struct_type_name;
                
                // parent_var.struct_membersに追加
                parent_var.struct_members[member.name] = nested_var;
                
                // 個別変数として登録
                interpreter_->current_scope().variables[member_path] = nested_var;
                
                // 再帰的にネストメンバを初期化
                initialize_nested_struct_members(
                    parent_var.struct_members[member.name],
                    member_path,
                    *nested_def
                );
            }
        }
    }
}
```

#### 1.2 ネストメンバへの代入サポート

**場所**: `src/backend/interpreter/executor/statement_executor.cpp` の `execute_member_assignment`

**変更点**:
- `obj.member1.member2 = value` の処理を正しく行う
- 再帰的にメンバアクセスチェーンを評価する

**実装**: 既に`evaluate_nested_member_access`関数があるので、それを活用する

#### 1.3 ネストメンバからの読み取りサポート

**場所**: `src/backend/interpreter/evaluator/expression_evaluator.cpp`

**変更点**:
- `obj.member1.member2` の評価を正しく行う
- `AST_MEMBER_ACCESS`の左側が`AST_MEMBER_ACCESS`の場合の処理

### Phase 2: `->` 演算子のサポート (優先度: 高)

#### 2.1 パーサーでの`->`トークン認識

**場所**: `src/frontend/recursive_parser/recursive_lexer.cpp`

**変更点**:
- `->` を新しいトークンタイプとして認識
- `TOK_ARROW` トークンを追加

#### 2.2 `->` を `(*p).` に変換

**場所**: `src/frontend/recursive_parser/recursive_parser.cpp`

**方針**:
- `p->member` を `(*p).member` に変換してASTを構築
- これにより、既存の`AST_MEMBER_ACCESS`と`AST_UNARY_OP`(DEREFERENCE)の仕組みを再利用できる

### Phase 3: selfを介したネストアクセス (優先度: 中)

**場所**: 
- `src/backend/interpreter/executor/statement_executor.cpp` の `execute_self_member_assignment`
- `src/backend/interpreter/evaluator/expression_evaluator.cpp` のselfメンバアクセス処理

**変更点**:
- `self.member1.member2` を正しく評価
- Phase 1のネストメンバサポートが完成すれば、自動的に動作するはず

### Phase 4: チェーン処理 (優先度: 中)

#### 4.1 メンバアクセスチェーン

**実装**: Phase 1で既に対応される

#### 4.2 メソッドチェーン

**例**: `obj.method1().method2()`

**実装**:
- 関数呼び出しの結果が構造体の場合、その構造体に対してさらにメンバアクセス可能にする
- `AST_FUNC_CALL`の親が`AST_MEMBER_ACCESS`の場合の処理

## 実装の優先順位

1. **Phase 1 (最優先)**: ネストした構造体メンバの基本サポート
   - これがないと、他のすべての機能が動作しない
   - 推定作業量: 中 (既存コードの修正が多い)

2. **Phase 2 (高)**: `->` 演算子のサポート
   - ユーザビリティの大幅な向上
   - 推定作業量: 小〜中 (シンタックスシュガーとして実装可能)

3. **Phase 3 (中)**: selfを介したネストアクセス
   - Phase 1が完成すれば自動的に動作する可能性が高い
   - 推定作業量: 小

4. **Phase 4 (中)**: チェーン処理
   - Phase 1, 2が完成すれば大部分が動作する
   - 推定作業量: 小〜中

## テストケース

### Test 1: 基本的なネストメンバ

```cb
struct Point { int x; int y; };
struct Box { Point topLeft; Point bottomRight; };

Box box;
box.topLeft.x = 10;
box.topLeft.y = 20;
box.bottomRight.x = 50;
box.bottomRight.y = 60;

println(box.topLeft.x);      // 10
println(box.bottomRight.y);  // 60
```

### Test 2: ポインタアクセス

```cb
Box* p = &box;
p->topLeft.x = 100;
println((*p).topLeft.x);     // 100
println(p->topLeft.x);       // 100
```

### Test 3: ネストしたポインタ

```cb
struct Container { Box* boxes; };
Container c;
c.boxes = &box;
c.boxes->topLeft.x = 200;
println(c.boxes->topLeft.x);  // 200
```

### Test 4: implでのselfアクセス

```cb
interface Geometry {
    int getArea();
};

impl Geometry for Box {
    int getArea() {
        int width = self.bottomRight.x - self.topLeft.x;
        int height = self.bottomRight.y - self.topLeft.y;
        return width * height;
    }
};

Box box = {{10, 20}, {50, 60}};
println(box.getArea());  // (50-10) * (60-20) = 40 * 40 = 1600
```

## 注意事項

- メモリ管理: ネストした構造体のコピー時の深いコピー vs 浅いコピー
- パフォーマンス: 深いネストによるオーバーヘッド
- エラーハンドリング: null ポインタ、未初期化メンバー
- 循環参照: 構造体Aがポインタ経由で構造体Bを参照し、BがAを参照する場合
