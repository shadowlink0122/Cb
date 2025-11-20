export default function techStack(): string {
    return `<section class="tech-stack-slide">
        <h2>技術スタック</h2>
        <div class="tech-grid-compact">
            <div class="tech-summary-compact">
                <p><strong>C++17実装のモダン言語処理系</strong><br/>
                インタープリタでとりあえず動かしたい<br/>
                コンパイラで高速化したい</p>
            </div>
            <div class="tech-card-compact">
                <h3><span class="tech-icon-inline">C++17</span> 実装言語</h3>
                <ul>
                    <li>モダンC++機能（variant, optional）</li>
                    <li>AIによるペアプログラミング支援</li>
                    <li>構造化束縛・テンプレート</li>
                    <li>高速コンパイル性能</li>
                </ul>
            </div>

            <div class="tech-card-compact">
                <h3><span class="tech-icon-inline">Dual Mode</span> 実行モデル</h3>
                <ul>
                    <li><strong>インタープリタ</strong>（現在稼働）</li>
                    <li>AST直接実行・即座に動作</li>
                    <li><strong>コンパイラ</strong>（開発中）</li>
                    <li>最適化バイナリ生成</li>
                </ul>
            </div>

            
        </div>
    </section>`;
}