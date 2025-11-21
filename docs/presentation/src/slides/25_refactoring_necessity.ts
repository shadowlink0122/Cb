export default function refactoringNecessity(): string {
    return `<section class="refactoring-necessity-slide">
        <h2>定期的なリファクタリングの必要性</h2>

        <div class="two-column-layout">
            <div class="column">
                <h3>⚠️ よくある問題</h3>
                <ul>
                    <li>1ファイル数千行のモンスターファイル</li>
                    <li>過度に複雑なロジック</li>
                    <li>重複コードの蔓延</li>
                    <li>テストが困難な構造</li>
                </ul>
            </div>

            <div class="column">
                <h3>✅ 解決策</h3>
                <ol>
                    <li><strong>動くものができた時点でリファクタリング</strong><br>「動く → 美しく」の2段階</li>
                    <li><strong>メンテナンス可能な粒度に分割</strong><br>1ファイル1000行以下を目安に</li>
                    <li><strong>AIと協力してリファクタリング</strong><br>「このファイルを分割して」</li>
                </ol>
            </div>
        </div>

        <div class="feature-note">
            <p>🔄 <strong>動作確認 → リファクタリング → テスト のサイクル</strong></p>
        </div>
    </section>`;
}