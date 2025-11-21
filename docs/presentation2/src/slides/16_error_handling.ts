export default function errorHandling(): string {
    return `<section class="error-handling-slide">
        <h2>エラーハンドリング - Rustライクな安全性</h2>

        <div class="error-handling-features">
            <div class="option-result-block">
                <h3>📦 Option型 - Null安全性</h3>
                <pre><code class="language-cb">// Option型でnullを安全に扱う（組み込み型）
Option<int> findValue(string key) {
    if (map.contains(key)) {
        return Option<int>::Some(map[key]);
    }
    return Option<int>::None;
}

// パターンマッチングで処理
Option<int> result = findValue("age");
match (result) {
    Some(value) => {
        println("Found: ", value);
    }
    None => {
        println("Not found");
    }
}

// ?演算子でエラー伝播（計画中）
int getValue() ? {
    int x = findValue("x")?;  // Noneなら即return
    int y = findValue("y")?;
    return x + y;
}</code></pre>
            </div>

            <div class="option-result-block">
                <h3>🎯 Result型 - エラー処理</h3>
                <pre><code class="language-cb">// Result型でエラーを明示的に扱う（組み込み型）
Result<int, string> divide(int a, int b) {
    if (b == 0) {
        return Result<int, string>::Err("Division by zero");
    }
    return Result<int, string>::Ok(a / b);
}

// 使用例
Result<int, string> res = divide(10, 2);
match (res) {
    Ok(value) => {
        println("Result: ", value);
    }
    Err(error) => {
        println("Error: ", error);
    }
}

// チェイン処理（計画中）
Result<int, string> calculate() {
    return divide(100, 5)
        .map(|x| x * 2)
        .and_then(|x| divide(x, 10));
}</code></pre>
            </div>

            <div class="exception-block">
                <h3>⚡ 例外処理</h3>
                <pre><code class="language-cb">// 従来の例外処理も可能
void riskyOperation() {
    if (errorCondition) {
        throw Error("Something went wrong");
    }
}

// try-catch-finally
try {
    riskyOperation();
    processData();
} catch (Error e) {
    println("Caught error: ", e.message);
    // エラーリカバリ
} finally {
    // クリーンアップ処理
    closeResources();
}

// カスタム例外型
struct FileError : Error {
    string filename;
    int errorCode;
};</code></pre>
            </div>
        </div>

        <div class="error-philosophy">
            <p>🛡️ <strong>Rustの安全性とC++の柔軟性の両立</strong> - 段階的に安全性を高められる</p>
        </div>
    </section>`;
}