export default function cbVision(): string {
    return `<section class="cb-vision-slide">
        <h2>Cbが目指すもの</h2>

        <div class="vision-container">
            <div class="vision-header">
                <p class="vision-statement">
                    <strong>低レベルから高レベルまで、1つの言語で</strong>
                </p>
            </div>

            <div class="capability-spectrum">
                <div class="low-level-card">
                    <h3>🔧 システムプログラミング</h3>
                    <div class="lang-comparison">
                        <span class="lang-tag cpp">C++</span>
                        <span class="lang-tag rust">Rust</span>
                        のように
                    </div>
                    <ul>
                        <li>OS・カーネル開発</li>
                        <li>デバイスドライバ</li>
                        <li>組み込みシステム</li>
                        <li>メモリ直接制御</li>
                        <li>ゼロコスト抽象化</li>
                    </ul>
                </div>

                <div class="arrow-both">
                    <span class="bidirectional">⟷</span>
                    <p>同じ言語で</p>
                </div>

                <div class="high-level-card">
                    <h3>🌐 アプリケーション開発</h3>
                    <div class="lang-comparison">
                        <span class="lang-tag typescript">TypeScript</span>
                        <span class="lang-tag go">Go</span>
                        のように
                    </div>
                    <ul>
                        <li>Webフロントエンド</li>
                        <li>バックエンドAPI</li>
                        <li>GUI アプリケーション</li>
                        <li>スクリプティング</li>
                        <li>高い生産性</li>
                    </ul>
                </div>
            </div>

            <div class="vision-footer">
                <p class="vision-goal">
                    <strong>究極の目標：</strong>
                    OSからWebアプリまで、すべてCbで書ける世界
                </p>
            </div>
        </div>
    </section>`;
}