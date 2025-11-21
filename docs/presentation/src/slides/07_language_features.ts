export default function languageFeatures(): string {
    return `<section class="language-features-slide">
        <h2>各言語のいいとこ取り</h2>

        <div class="features-container">
            <div class="features-intro">
                <p class="intro-text">
                    Cbは既存言語の優れた機能を組み合わせ、より使いやすく強力な言語を目指しています
                </p>
            </div>

            <div class="features-table">
                <table>
                    <thead>
                        <tr>
                            <th>言語</th>
                            <th>取り入れた機能</th>
                            <th>Cbでの実装</th>
                        </tr>
                    </thead>
                    <tbody>
                        <tr class="cpp-row">
                            <td class="lang-name">
                                <span class="lang-badge cpp">C++</span>
                            </td>
                            <td class="feature-list">
                                <ul>
                                    <li>テンプレート</li>
                                    <li>演算子オーバーロード</li>
                                    <li>ポインタ/参照</li>
                                    <li>ゼロコスト抽象化</li>
                                </ul>
                            </td>
                            <td class="implementation">
                                <ul>
                                    <li>ジェネリクス[T]</li>
                                    <li>メソッド定義での代替</li>
                                    <li>ポインタ(*T)と参照(&T)</li>
                                    <li>最適化コンパイル</li>
                                </ul>
                            </td>
                        </tr>
                        <tr class="rust-row">
                            <td class="lang-name">
                                <span class="lang-badge rust">Rust</span>
                            </td>
                            <td class="feature-list">
                                <ul>
                                    <li>所有権システム</li>
                                    <li>Result/Option型</li>
                                    <li>パターンマッチング</li>
                                    <li>トレイト</li>
                                </ul>
                            </td>
                            <td class="implementation">
                                <ul>
                                    <li>自動メモリ管理（計画中）</li>
                                    <li>Result[T,E] / Option[T]</li>
                                    <li>match式</li>
                                    <li>interface/impl</li>
                                </ul>
                            </td>
                        </tr>
                        <tr class="typescript-row">
                            <td class="lang-name">
                                <span class="lang-badge typescript">TypeScript</span>
                            </td>
                            <td class="feature-list">
                                <ul>
                                    <li>型システム</li>
                                    <li>構造的型付け</li>
                                    <li>ユニオン型</li>
                                    <li>async/await</li>
                                </ul>
                            </td>
                            <td class="implementation">
                                <ul>
                                    <li>明示的な型指定</li>
                                    <li>構造体の型システム</li>
                                    <li>Union型（|演算子）</li>
                                    <li>async/await（実装中）</li>
                                </ul>
                            </td>
                        </tr>
                        <tr class="go-row">
                            <td class="lang-name">
                                <span class="lang-badge go">Go</span>
                            </td>
                            <td class="feature-list">
                                <ul>
                                    <li>defer文</li>
                                    <li>複数戻り値</li>
                                    <li>エラーハンドリング</li>
                                    <li>シンプルな構文</li>
                                </ul>
                            </td>
                            <td class="implementation">
                                <ul>
                                    <li>defer（実装済み）</li>
                                    <li>複数戻り値（構造体で実現）</li>
                                    <li>Result型でのエラー処理</li>
                                    <li>シンプルな文法設計</li>
                                </ul>
                            </td>
                        </tr>
                        <tr class="python-row">
                            <td class="lang-name">
                                <span class="lang-badge python">Python</span>
                            </td>
                            <td class="feature-list">
                                <ul>
                                    <li>self キーワード</li>
                                    <li>リスト内包表記</li>
                                    <li>シンプルな構文</li>
                                </ul>
                            </td>
                            <td class="implementation">
                                <ul>
                                    <li>メソッドでのself使用</li>
                                    <li>配列操作（計画中）</li>
                                    <li>読みやすい構文</li>
                                </ul>
                            </td>
                        </tr>
                    </tbody>
                </table>
            </div>

            <div class="features-summary">
                <h3>🎯 Cbの独自性</h3>
                <div class="unique-features">
                    <div class="unique-item">
                        <strong>統一された文法</strong>
                        <p>各言語の機能を一貫性のある文法で提供</p>
                    </div>
                    <div class="unique-item">
                        <strong>段階的な学習</strong>
                        <p>基本から高度な機能まで段階的に習得可能</p>
                    </div>
                    <div class="unique-item">
                        <strong>実用性重視</strong>
                        <p>理論より実際の開発での使いやすさを優先</p>
                    </div>
                </div>
            </div>
        </div>
    </section>`;
}