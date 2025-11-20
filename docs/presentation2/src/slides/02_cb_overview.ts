export default function cbOverviewSlide(): string {
    return `<section class="cb-overview-slide">
        <h2>Cbとは？</h2>
        <div class="overview-container">
            <div class="highlight-box">
                <p class="main-description">
                    <strong>C++をベースに、Rust/Go/TypeScript/Pythonの<br/>
                    優れた機能を統合したモダンなシステムプログラミング言語</strong>
                </p>
            </div>
            <div class="features-grid">
                <div class="feature-card">
                    <h3>🎯 設計コンセプト</h3>
                    <ul>
                        <li>C/C++の親しみやすさとモダン言語の安全性</li>
                        <li>静的型付け + ジェネリクス</li>
                        <li>型安全な非同期プログラミング</li>
                    </ul>
                </div>
                <div class="feature-card">
                    <h3>🚀 主要機能</h3>
                    <ul>
                        <li>async/awaitによる非同期処理</li>
                        <li>Option/Result型によるエラーハンドリング</li>
                        <li>パターンマッチング</li>
                        <li>インターフェースとジェネリクス</li>
                    </ul>
                </div>
            </div>
        </div>
    </section>`;
}