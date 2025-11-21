export default function section1Summary(): string {
    return `<section class="section1-summary-slide">
        <h2>セクション1まとめ：Cbの言語設計</h2>

        <div class="two-column-layout">
            <div class="column">
                <h3>🎯 設計思想</h3>
                <ul>
                    <li><strong>C++をベースに</strong><br>静的型付け、高速な実行、システムプログラミング</li>
                    <li><strong>多言語からいいとこ取り</strong><br>モダン言語の優れた機能を統合</li>
                    <li><strong>実用性重視</strong><br>開発者の生産性とコードの安全性を両立</li>
                </ul>
            </div>

            <div class="column">
                <h3>🌟 採用した機能</h3>
                <ul style="font-size: 0.75em;">
                    <li><strong>Rust風</strong><br>implブロック、Option/Result型、パターンマッチング</li>
                    <li><strong>TypeScript風</strong><br>ユニオン型、リテラル型、型推論</li>
                    <li><strong>Go風</strong><br>Interface、defer</li>
                    <li><strong>Python風</strong><br>async/await、文字列補間</li>
                </ul>
            </div>
        </div>

        <div class="feature-note">
            <p>💡 <strong>「多言語のベストプラクティスを1つの言語に」</strong><br>
            それぞれの言語の良いところを取り入れた、実用的で安全な言語を目指す</p>
        </div>
    </section>`;
}
