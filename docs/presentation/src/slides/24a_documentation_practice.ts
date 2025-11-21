export default function documentationPractice(): string {
    return `<section class="documentation-practice-slide">
        <h2>実際のドキュメント管理方法</h2>

        <div class="doc-structure">
            <div class="doc-section">
                <h3>📋 バージョン管理</h3>
                <div class="doc-item">
                    <code>IMPLEMENTATION_PRIORITY.md</code>
                    <p>現在の実装優先度と進捗状況<br>
                    <span class="example">例: v0.14.0 統合テスト成功率 58% (493/849)</span></p>
                </div>
                <div class="doc-item">
                    <code>release_notes/v*.md</code>
                    <p>各バージョンのリリースノート<br>
                    <span class="example">v0.10.0, v0.11.0, v0.12.0, v0.13.0...</span></p>
                </div>
                <div class="doc-item">
                    <code>docs/archive/releases/</code>
                    <p>詳細な実装計画と報告書のアーカイブ</p>
                </div>
            </div>

            <div class="doc-section">
                <h3>✅ テストもドキュメント</h3>
                <div class="doc-item">
                    <code>tests/cases/</code>
                    <p><strong>実行可能な仕様書</strong>として機能<br>
                    849個のテストケースが言語仕様を表現</p>
                </div>
                <h3>🤖 このスライド自体も</h3>
                <div class="doc-item meta">
                    <code>docs/presentation2/</code>
                    <p>✨ <strong>AIによって生成・管理</strong><br>
                    Gitで履歴を追跡、変更も容易</p>
                </div>
            </div>
        </div>

        <div class="feature-note">
            <p>💡 <strong>ドキュメントもコードと同じ: バージョン管理 + 自動生成</strong></p>
        </div>

        <style>
            .documentation-practice-slide {
                padding: 2rem 3rem;
            }

            .doc-structure {
                display: grid;
                grid-template-columns: 1fr 1fr;
                gap: 2rem;
                margin-top: 2rem;
            }

            .doc-section {
                background: transparent;
                border-radius: 12px;
                padding: 1.5rem;
                border: 2px solid #3b82f6;
            }

            .doc-section h3 {
                margin: 0 0 1.2rem 0;
                color: #2c3e50;
                font-size: 1.6rem;
                font-weight: 600;
            }

            .doc-item {
                background: transparent;
                border-radius: 8px;
                padding: 1.2rem;
                margin-bottom: 1rem;
                border-left: 4px solid #60a5fa;
            }

            .doc-item.meta {
                border-left-color: #a78bfa;
            }

            .doc-item:last-child {
                margin-bottom: 0;
            }

            .doc-item h4 {
                margin: 0 0 0.8rem 0;
                color: #2c3e50;
                font-size: 1.3rem;
                font-weight: 600;
            }

            .doc-item code {
                display: block;
                color: #d97706;
                font-weight: bold;
                font-size: 1.15rem;
                margin-bottom: 0.6rem;
                font-family: 'Monaco', 'Menlo', monospace;
            }

            .doc-item p {
                margin: 0;
                color: #2c3e50;
                font-size: 1.05rem;
                line-height: 1.6;
            }

            .doc-item .example {
                display: block;
                color: #6b7280;
                font-size: 0.95rem;
                font-style: italic;
                margin-top: 0.4rem;
            }

            .feature-note {
                background: linear-gradient(135deg, #059669 0%, #10b981 100%);
                padding: 1rem;
                border-radius: 10px;
                margin-top: 1.5rem;
                border: 2px solid #34d399;
                text-align: center;
            }

            .feature-note p {
                margin: 0;
                color: #fff;
                font-size: 1.1rem;
            }
        </style>
    </section>`;
}
