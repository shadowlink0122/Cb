export default function patternMatching(): string {
    return `<section class="pattern-matching-slide">
        <h2>パターンマッチング</h2>

        <div class="code-section-large">
            <h3>🔍 強力な分岐機構</h3>
            <pre><code class="language-cb">// switch文の高度な機能
int processCode(int code) {
    switch (code) {
        case(1 | 2 | 3) {
            return code * 10;  // OR条件
        }
        case(10...20) {
            return code * 5;   // 範囲マッチング
        }
        else {
            return -1;
        }
    }
}

// match文（Option/Result用）
Result&lt;int, string&gt; result = divide(10, 2);
match (result) {
    Ok(value) =&gt; { println("成功: ", value); }
    Err(error) =&gt; { println("エラー: ", error); }
}</code></pre>
        </div>

        <div class="feature-note">
            <p>🎯 <strong>Rustライクな網羅的パターンマッチ</strong></p>
        </div>
    </section>`;
}