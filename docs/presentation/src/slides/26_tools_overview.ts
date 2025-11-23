export default function toolsOverview(): string {
    return `<section class="tools-overview-slide">
        <h2>AI開発を支える3つの武器</h2>

        <div class="two-column-layout">
            <div class="column">
                <h3>🧪 テスト</h3>
                <p><strong>意図通りに実装できているか検証</strong></p>
                <ul>
                    <li>AIのコード品質を確認</li>
                    <li>リファクタリングの安全性担保</li>
                    <li>回帰バグの早期発見</li>
                </ul>
            </div>

            <div class="column">
                <h3>🐛 デバッグモード & 🛡️ サニタイザー</h3>
                <ul>
                    <li><strong>デバッグモード</strong><br>実行フローを可視化</li>
                    <li><strong>サニタイザー</strong><br>メモリ・未定義動作を検出</li>
                    <li>詳細なログ出力、問題箇所の特定が容易</li>
                </ul>
            </div>
        </div>

        <div class="feature-note">
            <p>💡 <strong>開発しているプロジェクトの状態がAIにも人間にもわかるツールは便利</strong></p>
        </div>
    </section>`;
}