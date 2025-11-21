export default function costComparison(): string {
    return `<section class="cost-comparison-slide">
        <h2>💰 開発コストの現実</h2>

        <div class="two-column-layout">
            <div class="column">
                <h3>📊 開発期間とツール</h3>
                <ul>
                    <li><strong>開発期間</strong><br>2024年7月〜現在（3〜4ヶ月）</li>
                    <li><strong>GitHub Copilot Pro+</strong><br>$39/月（約¥7,000）<br><span class="highlight">開発当初から使用</span><br><span class="highlight">起床〜就寝まで使用でプレミアムリクエスト1500超</span></li>
                    <li><strong>Claude（$200プラン）</strong><br>$200/月（約¥30,000）<br><span class="highlight">今月から使用開始</span></li>
                </ul>
            </div>

            <div class="column">
                <h3>💡 コストパフォーマンス比較</h3>
                <div class="cost-summary">
                    <div class="cost-item copilot-winner">
                        <h4>✅ GitHub Copilot Pro+</h4>
                        <p class="price">¥7,000/月</p>
                        <ul>
                            <li>コード補完に特化</li>
                            <li>コスパ最強</li>
                            <li>日常的な開発に最適</li>
                            <li>終日使用で1500リクエスト超</li>
                        </ul>
                    </div>
                    <div class="cost-item">
                        <h4>⚠️ Claude $200プラン</h4>
                        <p class="price">¥30,000/月</p>
                        <ul>
                            <li>複雑な設計・リファクタリング</li>
                            <li>高コストだが高性能</li>
                            <li>特定タスクで威力発揮</li>
                        </ul>
                    </div>
                </div>
            </div>
        </div>

        <div class="feature-note">
            <p>💰 <strong>結論：コスパ的にはCopilotが圧勝。Claudeは必要な時だけ使うのが賢明</strong></p>
        </div>
    </section>`;
}
