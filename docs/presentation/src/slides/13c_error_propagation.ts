export default function errorPropagation(): string {
    return `<section class="error-propagation-slide">
        <h2>エラー伝播演算子</h2>

        <div class="code-section-large">
            <h3>⚡ Result/Option型の伝播</h3>
            <pre><code class="language-cb">// ? 演算子 - エラーの早期リターン
Result&lt;int, string&gt; calculate() {
    int x = getValue()?;      // Errなら即リターン
    int y = getAnother()?;    // Errなら即リターン
    return Result&lt;int, string&gt;::Ok(x + y);
}

// try式 - Result/Optionのアンラップ
Result&lt;int, string&gt; processValue() {
    int value = try getValue();  // 成功時は値を取得
    return Result&lt;int, string&gt;::Ok(value * 2);
}

// checked演算 - オーバーフロー検出
Result&lt;int, OverflowError&gt; safeAdd(int a, int b) {
    int sum = checked(a + b);  // オーバーフロー時はErr
    return Result&lt;int, OverflowError&gt;::Ok(sum);
}</code></pre>
        </div>

        <div class="feature-note">
            <p>🛡️ <strong>Rustライクな安全なエラー処理機構</strong></p>
        </div>
    </section>`;
}