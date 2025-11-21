export default function primitiveTypes(): string {
    return `<section class="primitive-types-slide">
        <h2>プリミティブ型</h2>

        <div class="two-column-layout">
            <div class="column">
                <h3>📊 整数型</h3>
                <ul>
                    <li><code>tiny</code> - 8ビット符号付き整数</li>
                    <li><code>short</code> - 16ビット符号付き整数</li>
                    <li><code>int</code> - 32ビット符号付き整数</li>
                    <li><code>long</code> - 64ビット符号付き整数</li>
                </ul>
                <h3>📊 浮動小数点型</h3>
                <ul>
                    <li><code>float</code> - 32ビット浮動小数点数</li>
                    <li><code>double</code> - 64ビット浮動小数点数</li>
                </ul>
            </div>

            <div class="column">
                <h3>💡 その他の基本型</h3>
                <ul>
                    <li><code>bool</code> - 真偽値（true/false）</li>
                    <li><code>char</code> - 文字型</li>
                    <li><code>void</code> - 値なし型</li>
                    <li><code>string</code> - 文字列型</li>
                </ul>
                <h3>🔍 型の特徴</h3>
                <ul>
                    <li>C/C++互換のサイズ</li>
                    <li>明確なビット幅</li>
                    <li>効率的なメモリ使用</li>
                </ul>
            </div>
        </div>

        <div class="feature-note">
            <p>🎯 <strong>C/C++と同じプリミティブ型で学習コストゼロ</strong></p>
        </div>
    </section>`;
}
