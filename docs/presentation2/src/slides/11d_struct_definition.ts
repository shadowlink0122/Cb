export default function structDefinition(): string {
    return `<section class="struct-definition-slide">
        <h2>構造体 - アクセス修飾子</h2>

        <div class="two-column-layout">
            <div class="column">
                <h3>🔒 private修飾子</h3>
                <pre style="font-size: 0.45em;"><code class="language-cb">struct Person {
    private string ssn;  // 外部から見えない
    string name;
    int age;
}

void example() {
    Person p;
    p.name = "Alice";    // OK
    p.age = 30;          // OK
    // p.ssn = "123-45";  // エラー！
}</code></pre>
                <ul style="font-size: 0.75em; margin-top: 1em;">
                    <li><strong>カプセル化</strong><br>外部から直接アクセスできない</li>
                    <li><strong>データ保護</strong><br>内部実装の隠蔽が可能</li>
                </ul>
            </div>

            <div class="column">
                <h3>⚡ default修飾子</h3>
                <pre style="font-size: 0.45em;"><code class="language-cb">struct Config {
    string host;
    int port;
    default string name = "default-server";
}
// ⚠️ default修飾子は1つだけ設定可能

void example() {
    Config c1;
    // c1.name = "default-server" (自動設定)
    c1.host = "localhost";  // 明示的に設定
    c1.port = 8080;         // 明示的に設定

    Config c2 = {
        host: "example.com",
        port: 3000,
        name: "custom-server"  // 上書き可能
    };
}</code></pre>
                <ul style="font-size: 0.75em; margin-top: 1em;">
                    <li><strong>1つのみ設定可能</strong><br>構造体に1つだけdefault指定</li>
                    <li><strong>初期化の簡略化</strong><br>デフォルト値を自動設定</li>
                    <li><strong>上書き可能</strong><br>必要に応じて明示的に設定</li>
                </ul>
            </div>
        </div>

        <div class="feature-note">
            <p>🛡️ <strong>privateで安全性を確保、defaultで利便性を向上</strong></p>
        </div>
    </section>`;
}
