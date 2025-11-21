export default function refactoringStory(): string {
    return `<section class="refactoring-story-slide">
        <h2>実例: Yacc/Lex → 手書きパーサへの移行</h2>

        <div class="two-column-layout">
            <div class="column">
                <h3>❌ 問題発生</h3>
                <ul>
                    <li>Yacc/Lexでは複雑な構文に対応困難</li>
                    <li>エラーメッセージが不親切</li>
                    <li>拡張性に限界</li>
                </ul>
            </div>

            <div class="column">
                <h3>🔄 移行の決断 → ✅ 移行成功</h3>
                <p><strong>手書き再帰下降パーサへ変更</strong></p>
                <ul>
                    <li>AIが既存テストを活用</li>
                    <li>200+のテストケースが安全網に</li>
                    <li>テストが通るまでAIが修正</li>
                    <li>全テスト通過で完了確認</li>
                </ul>
            </div>
        </div>

        <div class="feature-note">
            <p>🚀 <strong>テストがあれば大胆な技術選択も可能</strong></p>
        </div>
    </section>`;
}