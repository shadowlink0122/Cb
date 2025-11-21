export default function section2Summary(): string {
    return `<section class="section2-summary-slide">
        <h2>セクション2まとめ：AI開発の現実</h2>

        <div class="two-column-layout">
            <div class="column">
                <h3>🚀 AI開発の強み</h3>
                <ul>
                    <li><strong>アイデア以外は爆速</strong><br>実装・デバッグが圧倒的に速い</li>
                    <li><strong>デバッグ手法がAIにも有効</strong><br>テスト、デバッグモード、Sanitizer</li>
                    <li><strong>人間とAIの相乗効果</strong><br>より効率的な開発サイクル</li>
                </ul>
            </div>

            <div class="column">
                <h3>⚠️ ドキュメント戦略の落とし穴</h3>
                <ul>
                    <li><strong>短期的には有効</strong><br>AIに仕様を理解させやすい</li>
                    <li><strong>長期的な課題</strong><br>
                        ・AIがコンテキストを忘れる<br>
                        ・実装と仕様の乖離が発生
                    </li>
                    <li><strong>膨大なドキュメント量</strong><br>管理が困難に</li>
                </ul>
            </div>
        </div>

        <div class="feature-note">
            <p>💡 <strong>推奨アプローチ：人間用の必要最小限の仕様書 + リリースノート</strong><br>
            全体像を把握しやすく、メンテナンスも容易</p>
        </div>
    </section>`;
}
