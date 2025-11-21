export default function parserAndTest(): string {
    return `<section class="parser-test-slide">
        <h2>パーサとテストシステム</h2>

        <div class="features-container">
            <div class="parser-section">
                <h3>再帰下降パーサ</h3>
                <div class="parser-features">
                    <div class="feature-item">
                        <span class="feature-icon">📝</span>
                        <div>
                            <strong>手書き実装</strong>
                            <p>柔軟なエラーメッセージとリカバリー</p>
                        </div>
                    </div>
                    <div class="feature-item">
                        <span class="feature-icon">🚀</span>
                        <div>
                            <strong>高速解析</strong>
                            <p>パーサジェネレータ不要で直接的な実装</p>
                        </div>
                    </div>
                    <div class="feature-item">
                        <span class="feature-icon">🔄</span>
                        <div>
                            <strong>段階的パース</strong>
                            <p>構文→型推論→意味解析の明確な分離</p>
                        </div>
                    </div>
                </div>
            </div>

            <div class="test-section">
                <h3>開発支援機能</h3>
                <div class="test-features">
                    <div class="test-card">
                        <h4>実行モード</h4>
                        <code>./cb run script.cb</code>
                        <code>./cb compile script.cb</code>
                        <ul>
                            <li><strong>run:</strong> インタープリタ実行（主機能）</li>
                            <li><strong>compile:</strong> C++生成（開発中）</li>
                            <li>デバッグモード対応</li>
                        </ul>
                    </div>
                    <div class="test-card">
                        <h4>テストスイート</h4>
                        <code>make test</code>
                        <code>./run_unified_tests.sh</code>
                        <ul>
                            <li>850+のテストケース</li>
                            <li>インタープリタ/コンパイラ両対応</li>
                            <li>仕様変更の影響検証</li>
                        </ul>
                    </div>
                </div>
            </div>
        </div>

        <div class="stats-box">
            <span class="stat-item">✅ 443/849 tests passing</span>
            <span class="stat-item">📊 52.2% coverage</span>
            <span class="stat-item">🐛 早期バグ検出</span>
        </div>
    </section>`;
}