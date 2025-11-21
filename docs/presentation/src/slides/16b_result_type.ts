export default function resultType(): string {
    return `<section class="result-type-slide">
        <h2>Result型 - エラー処理</h2>

        <div class="code-section-large">
            <h3>🎯 組み込みResult型</h3>
            <pre><code class="language-cb">// Result型でエラーを明示的に扱う
Result&lt;int, string&gt; divide(int a, int b) {
    if (b == 0) {
        return Result&lt;int, string&gt;::Err("Division by zero");
    }
    return Result&lt;int, string&gt;::Ok(a / b);
}

// 使用例
Result&lt;int, string&gt; res = divide(10, 2);
match (res) {
    Ok(value) => {
        println("Result: ", value);
    }
    Err(error) => {
        println("Error: ", error);
    }
}

// チェイン処理（計画中）
Result&lt;int, string&gt; calculate() {
    return divide(100, 5)
        .map(|x| x * 2)
        .and_then(|x| divide(x, 10));
}</code></pre>
        </div>

        <div class="feature-note">
            <p>✅ <strong>エラーを値として扱い、明示的に処理</strong></p>
        </div>
    </section>`;
}