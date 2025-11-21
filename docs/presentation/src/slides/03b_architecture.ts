export default function architectureDiagram(): string {
    return `<section class="architecture-slide">
        <h2>実行アーキテクチャ</h2>

        <div class="architecture-flow">
            <div class="flow-container">
                <!-- 左側: メインフロー -->
                <div class="left-section">
                    <div class="main-flow">
                        <div class="source-box">Cbソースコード</div>
                        <div class="arrow-horizontal">→</div>
                        <div class="flow-box">再帰下降パーサ</div>
                        <div class="arrow-horizontal">→</div>
                        <div class="ast-box">AST生成</div>
                    </div>
                </div>

                <!-- 中央: 分岐矢印 -->
                <div class="branch-section">
                    <div class="arrow-up">↗</div>
                    <div class="arrow-down">↘</div>
                </div>

                <!-- 右側: 実行パス -->
                <div class="right-section">
                    <!-- インタープリタ（上） -->
                    <div class="interpreter-branch">
                        <div class="branch-header">インタープリタ（現在稼働中）</div>
                        <div class="branch-flow">
                            <div class="flow-box-interpreter">AST直接実行</div>
                            <div class="arrow-small">→</div>
                            <div class="flow-box-interpreter">C++ランタイム</div>
                            <div class="arrow-small">→</div>
                            <div class="flow-box-interpreter slow">実行結果<br/><small>（遅い）</small></div>
                        </div>
                    </div>

                    <!-- コンパイラ（下） -->
                    <div class="compiler-branch">
                        <div class="branch-header">コンパイラ（開発中）</div>
                        <div class="branch-flow">
                            <div class="flow-box-compiler">HIR変換</div>
                            <div class="arrow-small">→</div>
                            <div class="flow-box-compiler">C++コード生成</div>
                            <div class="arrow-small">→</div>
                            <div class="flow-box-compiler">最適化</div>
                            <div class="arrow-small">→</div>
                            <div class="flow-box-compiler fast">バイナリ<br/><small>（高速）</small></div>
                        </div>
                    </div>
                </div>
            </div>
        </div>

        <div class="performance-note">
            <p><strong>現状：</strong> インタープリタで開発中。最適化無しで低速だが、即座に実行可能</p>
            <p><strong>目標：</strong> コンパイラ完成でC++と同等の実行速度を実現</p>
        </div>
    </section>`;
}