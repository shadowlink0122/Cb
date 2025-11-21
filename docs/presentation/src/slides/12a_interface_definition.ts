export default function interfaceDefinition(): string {
    return `<section class="interface-definition-slide">
        <h2>Interface - トレイトシステム</h2>

        <div class="code-section-large">
            <h3>🔌 インターフェース定義</h3>
            <pre><code class="language-cb">// インターフェース定義（Rustのtraitに相当）
interface Drawable {
    void draw();
    Point getPosition();
}

interface Clickable {
    void onClick();
    bool isClickable();
}

// 構造体定義
struct Button {
    string label;
    Point position;
    bool enabled;
};</code></pre>
        </div>

        <div class="feature-note">
            <p>📐 <strong>振る舞いを定義する契約</strong></p>
        </div>
    </section>`;
}