export default function tableOfContents(): string {
    return `<section class="center-content">
        <h2>目次</h2>
        <div class="content-wrapper" style="max-width: 900px; margin: 0 auto;">
            <div class="toc-container" style="text-align: left; padding: 2em;">
                <div class="toc-item" style="margin-bottom: 2em;">
                    <h3 style="color: #3498db; font-size: 1.3em; margin-bottom: 0.5em;">📘 Cbとは</h3>
                    <p style="margin-left: 1.5em; color: #7f8c8d; font-size: 0.9em;">
                        言語の概要とビジョン
                    </p>
                </div>

                <div class="toc-item" style="margin-bottom: 2em;">
                    <h3 style="color: #e74c3c; font-size: 1.3em; margin-bottom: 0.5em;">🔧 Section 1: 実装した機能(インタプリタ)</h3>
                    <p style="margin-left: 1.5em; color: #7f8c8d; font-size: 0.9em;">
                        C/C++ライクな基本構文 + モダン言語の機能
                    </p>
                </div>

                <div class="toc-item" style="margin-bottom: 2em;">
                    <h3 style="color: #2ecc71; font-size: 1.3em; margin-bottom: 0.5em;">🤖 Section 2: バイブコーディング</h3>
                    <p style="margin-left: 1.5em; color: #7f8c8d; font-size: 0.9em;">
                        AI駆動開発による効率的な言語実装
                    </p>
                </div>

                <div class="toc-item" style="margin-bottom: 2em;">
                    <h3 style="color: #9b59b6; font-size: 1.3em; margin-bottom: 0.5em;">💡 Section 3: プロジェクトを通して学んだこと</h3>
                    <p style="margin-left: 1.5em; color: #7f8c8d; font-size: 0.9em;">
                        成功と失敗から得た教訓
                    </p>
                </div>
            </div>
        </div>
    </section>`;
}
