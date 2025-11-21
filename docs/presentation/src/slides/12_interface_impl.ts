export default function interfaceImpl(): string {
    return `<section class="interface-impl-slide">
        <h2>Interface & Impl - Rustライクなトレイト</h2>

        <div class="interface-examples">
            <div class="code-block-large">
                <h3>🔌 インターフェース定義と実装</h3>
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
}

// インターフェースの実装
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

            <div class="code-block-large">
                <h3>🎨 ポリモーフィズムの実現</h3>
                <pre><code class="language-cb">// インターフェース型として扱う
void renderUI(Drawable* items[], int count) {
    for (int i = 0; i < count; i++) {
        items[i]->draw();
    }
}

// 複数のインターフェースを要求
void handleInteraction(Drawable & Clickable widget) {
    widget.draw();
    if (widget.isClickable()) {
        widget.onClick();
    }
}

// 使用例
Button button = {
    label: "Submit",
    position: {x: 100, y: 200},
    enabled: true
};

// インターフェース型として扱える
Drawable* drawable = &button;
drawable->draw();

// 複数インターフェースも可能
handleInteraction(button);</code></pre>
            </div>
        </div>

        <div class="feature-highlight">
            <p>🚀 <strong>Rustのトレイトシステムの良さをCbに導入</strong></p>
        </div>
    </section>`;
}