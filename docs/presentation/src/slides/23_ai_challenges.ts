export default function aiChallenges(): string {
    return `<section class="ai-challenges-slide">
        <h2>AI開発の課題</h2>

        <div class="two-column-layout">
            <div class="column">
                <h3>⚠️ 膨大なドキュメント管理</h3>
                <ul>
                    <li>AIに任せるほど仕様書が増える</li>
                    <li>ドキュメントの一貫性維持が困難</li>
                    <li>更新コストの増大</li>
                </ul>
            </div>

            <div class="column">
                <h3>📦 ブラックボックス化</h3>
                <ul>
                    <li>自分でも理解していない部分が多い</li>
                    <li>コード生成のロジックが不透明</li>
                    <li>デバッグ時の困難</li>
                </ul>
            </div>
        </div>

        <div class="feature-note">
            <p>⚠️ <strong>1ファイル数千行の巨大ファイル生成</strong> - 保守性の低下</p>
            <p>⚠️ <strong>人間側の深い理解が必要</strong> - プロジェクト全体の熟知が必須</p>
        </div>
    </section>`;
}