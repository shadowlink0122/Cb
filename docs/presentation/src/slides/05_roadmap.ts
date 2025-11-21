export default function roadmap(): string {
    return `<section class="roadmap-slide">
        <h2>開発ロードマップ</h2>

        <div class="timeline-container-compact">
            <div class="timeline-phase completed">
                <div class="phase-header">
                    <span class="phase-status">✅</span>
                    <span class="phase-number">Phase 1</span>
                </div>
                <h3>基本構文</h3>
                <p>変数・関数・制御構造<br/>型システム・配列<br/>インターフェース<br/>トレイト</p>
            </div>

            <div class="timeline-phase planned">
                <div class="phase-header">
                    <span class="phase-status">📝</span>
                    <span class="phase-number">Phase 3</span>
                </div>
                <h3>高度な機能</h3>
                <p>ジェネリクス<br/>FFI<br/>マクロシステム<br/>（インタープリタ実装済）</p>
            </div>

            <div class="timeline-phase in-progress">
                <div class="phase-header">
                    <span class="phase-status">🚧</span>
                    <span class="phase-number">Phase 2</span>
                </div>
                <h3>コンパイラ化</h3>
                <p>HIR→C++変換<br/>バイナリ生成</p>
            </div>

            <div class="timeline-phase future">
                <div class="phase-header">
                    <span class="phase-status">🔮</span>
                    <span class="phase-number">Phase 4</span>
                </div>
                <h3>エコシステム</h3>
                <p>パッケージ管理<br/>標準ライブラリ<br/>IDE統合</p>
            </div>
        </div>

        <div class="version-info">
            <span class="current-version">現在: v0.13.1</br>インタプリタ終了</span>
            <span class="current-version">現在: v0.14.0</br>コンパイラ開始</span>
            <span class="next-milestone">次期: v1.0.0</br>エコシステム完成</span>
        </div>
    </section>`;
}