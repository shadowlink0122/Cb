export default function enumType(): string {
    return `<section class="enum-slide">
        <h2>Enum（列挙型）</h2>

        <div class="code-section-large">
            <h3>🎲 型安全な定数定義</h3>
            <pre><code class="language-cb">// 値を指定したEnum
enum Status {
    OK = 200,
    NOT_FOUND = 404,
    ERROR = 500
}

// 自動採番されるEnum
enum Color { RED, GREEN, BLUE }

// 使用例
Status code = Status::OK;
if (code == Status::NOT_FOUND) {
    println("Not found");
}

Color myColor = Color::RED;
switch (myColor) {
    case(Color::RED):   println("赤色"); break;
    case(Color::GREEN): println("緑色"); break;
    case(Color::BLUE):  println("青色"); break;
}</code></pre>
        </div>

        <div class="feature-note">
            <p>🏷️ <strong>Cライクな列挙型で可読性向上</strong></p>
        </div>
    </section>`;
}