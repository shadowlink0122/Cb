export default function sanitizerIntro(): string {
    return `<section class="sanitizer-intro-slide">
        <h2>サニタイザー - メモリ安全性の番人</h2>

        <div class="two-column-layout">
            <div class="column">
                <h3>🛡️ サニタイザーとは</h3>
                <p><strong>実行時にメモリエラーや未定義動作を検出</strong></p>
                <ul>
                    <li><strong>AddressSanitizer (ASan)</strong><br>メモリリーク・バッファオーバーフロー</li>
                    <li><strong>UndefinedBehaviorSanitizer (UBSan)</strong><br>未定義動作</li>
                    <li><strong>ThreadSanitizer (TSan)</strong><br>データ競合</li>
                </ul>
            </div>

            <div class="column">
                <h3>💡 AI開発での重要性</h3>
                <ul>
                    <li>AIが気づかないメモリエラー発見</li>
                    <li>セグフォの原因を即座に特定</li>
                    <li>未定義動作の早期検出</li>
                    <li>バグ修正時間の大幅短縮</li>
                </ul>
            </div>
        </div>

        <div class="feature-note">
            <p>🔧 <strong>コンパイル時に -fsanitize=address で有効化</strong></p>
        </div>
    </section>`;
}