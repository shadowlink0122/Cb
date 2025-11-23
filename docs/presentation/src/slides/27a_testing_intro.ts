export default function testingIntro(): string {
    return `<section class="testing-intro-slide">
        <h2>テスト - AI開発の品質保証</h2>

        <div class="two-column-layout">
            <div class="column">
                <h3>🎯 テストの役割</h3>
                <ul>
                    <li>AIの実装が仕様通りか確認</li>
                    <li>リファクタリング時の安全網</li>
                    <li>回帰バグの防止</li>
                    <li>ドキュメントとしての機能</li>
                </ul>
            </div>

            <div class="column">
                <h3>✨ Cbプロジェクトでの実践</h3>
                <p><strong>200個以上のテストケースを用意</strong></p>
                <ul>
                    <li>基本構文テスト</li>
                    <li>型システムテスト</li>
                    <li>HIR変換テスト</li>
                    <li>コード生成テスト</li>
                </ul>
            </div>
        </div>

        <div class="feature-note">
            <p>🚀 <strong>テストがあるから大胆にリファクタリングできる</strong></p>
        </div>
    </section>`;
}