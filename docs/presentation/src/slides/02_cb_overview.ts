export default function cbOverviewSlide(): string {
    return `<section class="cb-overview-slide">
        <h2>Cbとは？</h2>
        <div class="overview-container">
            <div class="highlight-box">
                <p class="main-description">
                    <strong>
                        読み方はシーフラット<br/>
                        C++をベースに、Rust/Go/TypeScript/Pythonの<br/>
                        優れた機能を統合したモダンなシステムプログラミング言語
                    </strong>
                </p>
            </div>
            <div class="features-grid">
                <div class="feature-card">
                    <h3>🎯 設計コンセプト</h3>
                    <ul>
                        <li>C/C++の親しみやすさ</li>
                        <li>モダンな言語の書きやすさ</li>
                        <li>静的型付け + ジェネリクス</li>
                        <li>インタープリタで即座に実行（現在）</li>
                        <li>コンパイラでバイナリ生成（開発中）</li>
                    </ul>
                </div>
                <div class="feature-card">
                    <h3>🚀 主要機能</h3>
                    <ul>
                        <li>基本構文はいろんな言語のいいとこ取り</li>
                        <li>Option/Result型によるエラーハンドリング</li>
                        <li>interface/implとジェネリクス</li>
                        <li>FFIで他言語と連携可能</li>
                    </ul>
                </div>
            </div>
        </div>
    </section>`;
}