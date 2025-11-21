export default function aiDevelopmentIntro(): string {
    return `<section class="ai-development-intro-slide">
        <h2>AIによる開発の実態</h2>

        <div class="two-column-layout">
            <div class="column">
                <h3>🤖 Cbプロジェクトの開発体制</h3>
                <ul>
                    <li><strong>設計・ドキュメント作成</strong><br>ほぼすべてAI</li>
                    <li><strong>コード実装</strong><br>90%以上をAIが生成</li>
                    <li><strong>テスト作成</strong><br>AIが自動生成</li>
                    <li><strong>リファクタリング</strong><br>AIと人間の協働</li>
                </ul>
            </div>

            <div class="column">
                <h3>💡 使用しているAIツール</h3>
                <ul>
                    <li><strong>GitHub Copilot</strong><br>コード補完・生成</li>
                    <li><strong>Claude Code</strong><br>複雑な実装・リファクタリング</li>
                    <li><strong>よく使うモデル</strong><br>Claude Sonnet4.5</li>
                </ul>
            </div>
        </div>

        <div class="feature-note">
            <p>⚡ <strong>人間1人 + AI = 大規模プロジェクト開発が可能</strong></p>
        </div>
    </section>`;
}