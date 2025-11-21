export default function exportModule(): string {
    return `<section class="export-module-slide">
        <h2>モジュールシステム - export</h2>

        <div class="code-section-large">
            <h3>📤 モジュールのエクスポート</h3>
            <pre><code class="language-cb">// 関数のエクスポート
export int add(int a, int b) {
    return a + b;
}

// 型のエクスポート
export struct Point {
    long x,
    long y
}

// 複数の要素をまとめてエクスポート
export {
    multiply,
    divide,
    Vector,
    Matrix
};

// デフォルトエクスポート
export default int main() {
    return 0;
}</code></pre>
        </div>

        <div class="feature-note">
            <p>📦 <strong>明示的なエクスポートで公開APIを制御</strong></p>
        </div>
    </section>`;
}
