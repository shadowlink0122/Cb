export default function documentationStrategy(): string {
    return `<section class="documentation-strategy-slide">
        <h2>ドキュメント戦略のトレードオフ</h2>

        <div class="two-column-layout">
            <div class="column">
                <h3>✅ メリット</h3>
                <ul>
                    <li><strong>プロンプトが簡潔に</strong><br>「仕様書通りに実装して」で済む</li>
                    <li><strong>一貫性の維持</strong><br>全員が同じ仕様を参照</li>
                    <li><strong>AIの理解が深まる</strong><br>コンテキストを共有しやすい</li>
                </ul>
            </div>

            <div class="column">
                <h3>❌ デメリット</h3>
                <ul>
                    <li><strong>管理コストの増大</strong><br>ドキュメント量が膨大に</li>
                    <li><strong>更新の遅れ</strong><br>実装とドキュメントの乖離</li>
                    <li><strong>何が最新かわからない</strong><br>バージョン管理の複雑化</li>
                </ul>
            </div>
        </div>

        <div class="feature-note">
            <p>⚖️ <strong>必要最小限のドキュメント + コードベースの真実</strong></p>
        </div>
    </section>`;
}