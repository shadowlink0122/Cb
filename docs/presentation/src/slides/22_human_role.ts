export default function humanRole(): string {
    return `<section class="human-role-slide">
        <h2>人間にしかできないこと</h2>

        <div class="two-column-layout">
            <div class="column">
                <h3>🎯 言語設計の思想・ビジョン</h3>
                <p><strong>「どういう言語にしたいか」という方向性</strong></p>
                <ul>
                    <li>言語の哲学・コンセプト</li>
                    <li>ターゲットユーザー像</li>
                    <li>機能の優先順位</li>
                    <li>トレードオフの判断</li>
                </ul>
            </div>

            <div class="column">
                <h3>⚖️ 重要な意思決定</h3>
                <ul>
                    <li>アーキテクチャの選択</li>
                    <li>技術スタックの決定</li>
                    <li>パフォーマンスと可読性のバランス</li>
                    <li>実装方針の転換（例: Yacc/Lex → 手書きパーサ）</li>
                </ul>
            </div>
        </div>

        <div class="feature-note">
            <p>💡 <strong>AIは手段、方向性を決めるのは人間</strong></p>
        </div>
    </section>`;
}