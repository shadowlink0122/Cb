# defer文の実装設計

**機能**: Go言語風のdefer文  
**優先度**: 高  
**実装期間**: Week 1-2  
**作成日**: 2025年10月11日

---

## 📋 概要

defer文は、スコープ終了時に実行される文を登録する機能です。
複数のdefer文がある場合、登録された逆順（LIFO: Last In First Out）で実行されます。

---

## 🎯 仕様

### 基本動作

```cb
void main() {
    defer println("3: 最後に実行");
    defer println("2: 2番目に実行");
    println("1: 最初に実行");
}
// 出力:
// 1: 最初に実行
// 2: 2番目に実行
// 3: 最後に実行
```

### スコープごとの実行

```cb
void main() {
    defer println("main終了");
    
    {
        defer println("ブロック1終了");
        println("ブロック1内");
    }  // ここで"ブロック1終了"が実行される
    
    {
        defer println("ブロック2終了");
        println("ブロック2内");
    }  // ここで"ブロック2終了"が実行される
    
    println("main内");
}  // ここで"main終了"が実行される
```

### return時の実行

```cb
int openFile(string path) {
    FILE* fp = fopen(path, "r");
    if (fp == null) {
        return -1;  // defer文なし、すぐにreturn
    }
    
    defer fclose(fp);  // この後のすべてのreturnでfcloseが実行される
    
    // 処理...
    if (error) {
        return -2;  // returnの前にfcloseが実行される
    }
    
    return 0;  // returnの前にfcloseが実行される
}
```

### ループ内のdefer

```cb
void processFiles() {
    for (int i = 0; i < 3; i = i + 1) {
        defer println("ループ終了: " + i);
        println("ループ内: " + i);
    }
    // 各イテレーションの終了時にdefer実行
}
// 出力:
// ループ内: 0
// ループ終了: 0
// ループ内: 1
// ループ終了: 1
// ループ内: 2
// ループ終了: 2
```

### 関数呼び出しの評価タイミング

```cb
int getValue() {
    println("getValue呼び出し");
    return 42;
}

void main() {
    int x = 10;
    defer println("値: " + getValue());  // getValue()はここで評価される
    x = 20;
}
// 出力:
// getValue呼び出し
// 値: 42
```

**重要**: defer文に渡される引数は、**defer文が実行された時点**で評価されます。

### 複雑な例: リソース管理

```cb
void processData(string file1, string file2) {
    FILE* fp1 = fopen(file1, "r");
    if (fp1 == null) return;
    defer fclose(fp1);
    
    FILE* fp2 = fopen(file2, "w");
    if (fp2 == null) return;  // fp1は自動的にクローズされる
    defer fclose(fp2);
    
    // データ処理
    // どのreturnパスでも両方のファイルが確実にクローズされる
}
```

---

## 🏗️ 実装設計

### 1. AST構造

```cpp
// ast.h に追加
struct DeferNode : public ASTNode {
    std::unique_ptr<ASTNode> statement;  // defer対象の文
    
    DeferNode(std::unique_ptr<ASTNode> stmt)
        : ASTNode(NodeType::Defer), statement(std::move(stmt)) {}
};
```

### 2. Lexer拡張

```cpp
// Lexer::getNextToken() に追加
if (identifier == "defer") {
    return Token(TokenType::DEFER, "defer");
}
```

`TokenType`に`DEFER`を追加：
```cpp
enum class TokenType {
    // ...既存のトークン...
    DEFER,
    // ...
};
```

### 3. Parser拡張

```cpp
// Parser.h に追加
std::unique_ptr<ASTNode> parseDefer();
```

```cpp
// Parser.cpp に実装
std::unique_ptr<ASTNode> Parser::parseDefer() {
    expect(TokenType::DEFER);  // 'defer' キーワード
    
    // defer対象の文を解析（式文、関数呼び出しなど）
    auto stmt = parseStatement();
    
    return std::make_unique<DeferNode>(std::move(stmt));
}
```

`parseStatement()`に追加：
```cpp
std::unique_ptr<ASTNode> Parser::parseStatement() {
    // ...既存の処理...
    
    if (currentToken.type == TokenType::DEFER) {
        return parseDefer();
    }
    
    // ...既存の処理...
}
```

### 4. インタープリタ実装

#### 4.1 Deferスタックの管理

```cpp
// Interpreter.h に追加
class Interpreter {
private:
    // 各スコープごとのdeferスタック
    std::vector<std::vector<ASTNode*>> deferStacks;
    
public:
    void pushDeferScope();      // 新しいスコープ開始
    void popDeferScope();       // スコープ終了、defer実行
    void addDefer(ASTNode* stmt);  // defer文の登録
    void executeDeferStack();   // 現在のスコープのdeferを実行
};
```

#### 4.2 Deferスタックの実装

```cpp
// Interpreter.cpp
void Interpreter::pushDeferScope() {
    deferStacks.push_back(std::vector<ASTNode*>());
}

void Interpreter::popDeferScope() {
    if (deferStacks.empty()) return;
    
    // LIFO順で実行
    auto& currentStack = deferStacks.back();
    for (auto it = currentStack.rbegin(); it != currentStack.rend(); ++it) {
        evaluate(*it);
    }
    
    deferStacks.pop_back();
}

void Interpreter::addDefer(ASTNode* stmt) {
    if (!deferStacks.empty()) {
        deferStacks.back().push_back(stmt);
    }
}

void Interpreter::executeDeferStack() {
    popDeferScope();
}
```

#### 4.3 evaluate()の拡張

```cpp
std::any Interpreter::evaluate(ASTNode* node) {
    // ...既存の処理...
    
    if (auto deferNode = dynamic_cast<DeferNode*>(node)) {
        // defer文を現在のスコープに登録
        addDefer(deferNode->statement.get());
        return {};
    }
    
    // ...既存の処理...
}
```

#### 4.4 スコープ管理の統合

```cpp
// ブロック文の評価
std::any Interpreter::evaluateBlock(BlockNode* node) {
    pushDeferScope();  // スコープ開始
    
    for (auto& stmt : node->statements) {
        auto result = evaluate(stmt.get());
        
        // return文の場合
        if (isReturning) {
            popDeferScope();  // deferを実行してからreturn
            return result;
        }
    }
    
    popDeferScope();  // スコープ終了、defer実行
    return {};
}

// 関数呼び出しの評価
std::any Interpreter::evaluateFunctionCall(FunctionCallNode* node) {
    pushDeferScope();  // 関数スコープ開始
    
    // 関数本体の実行
    auto result = evaluateFunctionBody(node);
    
    popDeferScope();  // 関数終了、defer実行
    return result;
}

// ループの評価（各イテレーションごと）
std::any Interpreter::evaluateForLoop(ForLoopNode* node) {
    // 初期化
    evaluate(node->init.get());
    
    while (evaluateCondition(node->condition.get())) {
        pushDeferScope();  // イテレーションスコープ開始
        
        evaluate(node->body.get());
        
        popDeferScope();  // イテレーション終了、defer実行
        
        evaluate(node->increment.get());
    }
    
    return {};
}
```

---

## 🧪 テストケース

### test_defer_basic.cb
```cb
// 基本的なdefer
void main() {
    defer println("3");
    defer println("2");
    println("1");
}
// 期待出力: 1, 2, 3
```

### test_defer_scope.cb
```cb
// スコープごとのdefer
void main() {
    defer println("main");
    {
        defer println("block1");
        println("in block1");
    }
    println("after block1");
}
// 期待出力: in block1, block1, after block1, main
```

### test_defer_return.cb
```cb
// return時のdefer実行
int test(int x) {
    defer println("cleanup");
    
    if (x < 0) {
        return -1;
    }
    
    return x * 2;
}

void main() {
    println(test(5));
    println(test(-5));
}
// 期待出力: cleanup, 10, cleanup, -1
```

### test_defer_loop.cb
```cb
// ループ内のdefer
void main() {
    for (int i = 0; i < 3; i = i + 1) {
        defer println("end: " + i);
        println("start: " + i);
    }
}
// 期待出力: start: 0, end: 0, start: 1, end: 1, start: 2, end: 2
```

### test_defer_evaluation.cb
```cb
// 引数の評価タイミング
int getValue() {
    println("getValue");
    return 42;
}

void main() {
    int x = 10;
    defer println("x = " + x);
    defer println("value = " + getValue());
    x = 20;
    println("main: x = " + x);
}
// 期待出力: getValue, main: x = 20, value = 42, x = 10
```

### test_defer_resource.cb
```cb
// リソース管理のシミュレーション
struct Resource {
    int id;
}

Resource* open(int id) {
    println("open: " + id);
    Resource* r = malloc(sizeof(Resource));
    r->id = id;
    return r;
}

void close(Resource* r) {
    println("close: " + r->id);
    free(r);
}

void process() {
    Resource* r1 = open(1);
    defer close(r1);
    
    Resource* r2 = open(2);
    defer close(r2);
    
    println("processing");
}

void main() {
    process();
    println("done");
}
// 期待出力: open: 1, open: 2, processing, close: 2, close: 1, done
```

---

## ✅ 実装チェックリスト

### Phase 1: 基本構造 (Day 1-2)
- [ ] `TokenType::DEFER` をLexerに追加
- [ ] `DeferNode` クラスをASTに追加
- [ ] `parseDefer()` をParserに追加
- [ ] 基本的なパースのテスト

### Phase 2: インタープリタ (Day 3-5)
- [ ] Deferスタックの実装
- [ ] `pushDeferScope()`, `popDeferScope()` の実装
- [ ] `addDefer()` の実装
- [ ] `evaluate(DeferNode*)` の実装

### Phase 3: スコープ統合 (Day 6-8)
- [ ] ブロック文でのdefer実行
- [ ] 関数でのdefer実行
- [ ] return文でのdefer実行
- [ ] ループでのdefer実行

### Phase 4: テストと検証 (Day 9-10)
- [ ] 基本的なdeferのテスト
- [ ] スコープごとのテスト
- [ ] return時のテスト
- [ ] ループ内のテスト
- [ ] 引数評価タイミングのテスト
- [ ] リソース管理のテスト

### Phase 5: ドキュメント (Day 11-14)
- [ ] 使用例の追加
- [ ] エッジケースの文書化
- [ ] サンプルコードの作成

---

## 🚨 注意事項

### エッジケース

1. **ネストしたdefer**:
```cb
void main() {
    defer {
        defer println("inner");
        println("outer");
    }
}
// 出力: outer, inner
```

2. **defer内でのreturn禁止**:
```cb
int test() {
    defer return 42;  // エラー: deferからreturnできない
    return 0;
}
```

3. **defer内での変数キャプチャ**:
```cb
void main() {
    int x = 10;
    defer println(x);  // xは10と評価される（defer時点）
    x = 20;
}
// 出力: 10
```

### パフォーマンス考慮

- Deferスタックはスコープごとに管理
- 大量のdefer文がある場合のメモリ使用量に注意
- LIFO実行のオーバーヘッドは最小限

---

## 📚 参考資料

- [Go言語のdefer文](https://go.dev/blog/defer-panic-and-recover)
- [Zig言語のdefer](https://ziglang.org/documentation/master/#defer)
- [Swift言語のdefer](https://docs.swift.org/swift-book/documentation/the-swift-programming-language/statements/#Defer-Statement)

---

## 🔄 次のステップ

1. ✅ 詳細設計完了
2. ⏳ Lexerの実装開始
3. ⏳ Parserの実装
4. ⏳ インタープリタの実装
5. ⏳ テストケース作成
6. ⏳ 動作検証
