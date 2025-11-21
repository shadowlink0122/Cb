export default function debugModeIntro(): string {
    return `<section class="debug-mode-intro-slide">
        <h2>デバッグモード - 実行フローの可視化</h2>

        <div class="two-column-layout">
            <div class="column">
                <h3>🔍 デバッグモードとは</h3>
                <p><strong>プログラムの実行過程を詳細にログ出力</strong></p>
                <ul>
                    <li>関数呼び出しのトレース</li>
                    <li>変数の値の変化</li>
                    <li>条件分岐の判定結果</li>
                    <li>エラーが発生した位置</li>
                </ul>
            </div>

            <div class="column">
                <h3>💡 なぜ必要か</h3>
                <ul>
                    <li>AIが生成したコードの動作確認</li>
                    <li>バグの原因を素早く特定</li>
                    <li>複雑なロジックの理解</li>
                    <li>本番環境での問題調査</li>
                </ul>
            </div>
        </div>

        <div class="feature-note">
            <p>🛠️ <strong>--debug などのオプションで詳細ログを有効化</strong></p>
        </div>
    </section>`;
}