export default function switchStatement(): string {
    return `<section class="switch-statement-slide">
        <h2>switch文 - 強化された条件分岐</h2>

        <div class="code-section-large">
            <h3>🔀 多彩な条件指定</h3>
            <pre><code class="language-cb">
switch (num) {
    // 単一条件
    case(1){
        println("1");
    }
    // OR条件（|）- いずれかにマッチ
    case(2 | 3){
        println("2 or 3");
    }
    // 範囲条件（...）- 範囲内にマッチ
    case(4...6){
        println("4 to 6");
    }
    // 複合条件も可能
    case(7 | 10...12){
        println("7 or 10 to 12");
    }
    // それ以外
    else{
        println("Other");
    }
}</code></pre>
        </div>

        <div class="feature-note">
            <p>⚡ <strong>柔軟な条件指定で複雑な分岐を簡潔に表現</strong></p>
        </div>
    </section>`;
}