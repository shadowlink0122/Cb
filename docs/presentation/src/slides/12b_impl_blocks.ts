export default function implBlocks(): string {
    return `<section class="impl-blocks-slide">
        <h2>Impl - インターフェースの実装</h2>

        <div class="code-section-large">
            <h3>⚙️ 実装ブロック</h3>
            <pre><code class="language-cb">// インターフェースの実装
impl Drawable for Button {
    void draw() {
        println("Drawing button: ", this.label);
        // 描画処理
    }

    Point getPosition() {
        return this.position;
    }
}

impl Clickable for Button {
    void onClick() {
        if (this.enabled) {
            println("Button clicked: ", this.label);
        }
    }

    bool isClickable() {
        return this.enabled;
    }
}</code></pre>
        </div>

        <div class="feature-note">
            <p>🚀 <strong>複数インターフェースの実装が可能</strong></p>
        </div>
    </section>`;
}