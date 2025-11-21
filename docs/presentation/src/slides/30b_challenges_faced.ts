export default function challengesFaced(): string {
    return `<section class="challenges-faced-slide">
        <h2>課題・ダメだったこと</h2>

        <div class="two-column-layout">
            <div class="column">
                <h3>⚠️ AI開発の限界</h3>
                <ul>
                    <li><strong>アーキテクチャ設計</strong><br>大局的な設計判断は人間が必要</li>
                    <li><strong>複雑なバグ</strong><br>AIだけでは解決困難な問題も</li>
                    <li><strong>コンテキストの喪失</strong><br>長期プロジェクトで情報が散逸</li>
                    <li><strong>品質のブレ</strong><br>生成されたコードの質にばらつき</li>
                </ul>
            </div>

            <div class="column">
                <h3>🔧 技術的課題</h3>
                <ul>
                    <li><strong>Yaccの限界</strong><br>複雑な言語仕様には不向き</li>
                    <li><strong>デバッグの難しさ</strong><br>生成されたコードの理解に時間</li>
                    <li><strong>パフォーマンス最適化</strong><br>AIは最適解を出せない場合も</li>
                    <li><strong>ドキュメントの肥大化</strong><br>管理が困難になりがち</li>
                    <li><strong>テストカバレッジ</strong><br>網羅的なテストは人間が設計</li>
                </ul>
            </div>
        </div>

        <div class="feature-note">
            <p>💡 <strong>AIは強力なツールだが、人間の判断と管理が不可欠</strong></p>
        </div>
    </section>`;
}
