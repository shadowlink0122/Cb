export default function polymorphism(): string {
    return `<section class="polymorphism-slide">
        <h2>ポリモーフィズムの実現</h2>

        <div class="code-section-large">
            <h3>🎨 インターフェース型として扱う</h3>
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
drawable->draw();</code></pre>
        </div>

        <div class="feature-note">
            <p>🔄 <strong>Rustのトレイトシステムの良さをCbに導入</strong></p>
        </div>
    </section>`;
}