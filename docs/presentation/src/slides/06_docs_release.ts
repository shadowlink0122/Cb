export default function docsAndRelease(): string {
    return `<section class="docs-release-slide">
        <h2>ドキュメントとリリース管理</h2>

        <div class="docs-release-grid">
            <div class="docs-section-compact">
                <h3>📚 ドキュメント</h3>
                <div class="doc-item">
                    <strong>言語仕様書</strong>
                    <span class="status-badge warning">部分メンテナンス</span>
                    <p>構文定義・型システム・標準API</p>
                </div>
                <div class="doc-item">
                    <strong>実装優先度</strong>
                    <span class="status-badge success">アクティブ</span>
                    <p>機能優先順位・実装状況追跡</p>
                </div>
            </div>

            <div class="release-section-compact">
                <h3>🚀 リリース管理</h3>
                <div class="release-item">
                    <strong>GitHubタグ</strong>
                    <div class="tag-list-compact">
                        <span class="tag">v0.13.1</span>
                        <span class="tag">v0.13.0</span>
                        <span class="tag">v0.12.0</span>
                    </div>
                </div>
                <div class="release-item">
                    <strong>リリースノート</strong>
                    <p>✨ 新機能 🐛 バグ修正 📊 テスト状況</p>
                </div>
            </div>
        </div>

        <div class="github-info-compact">
            <code>github.com/shadowlink0122/Cb</code>
            <span>｜</span>
            <span>500+ commits</span>
            <span>｜</span>
            <span>v0.13.1</span>
        </div>
    </section>`;
}