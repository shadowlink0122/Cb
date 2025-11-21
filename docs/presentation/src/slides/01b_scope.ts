export default function scopeSlide(): string {
    return `<section class="center-content">
        <h2>本日お話しすること / しないこと</h2>
        <div class="content-wrapper" style="max-width: 90%; margin: 0 auto;">
            <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 3em; padding: 1em;">
                <!-- 話すこと -->
                <div style="background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); padding: 1.5em; border-radius: 12px; color: white;">
                    <h3 style="color: white; margin-bottom: 1.5em; font-size: 1.3em;">✅ お話しすること</h3>
                    <ul style="text-align: left; font-size: 0.95em; line-height: 2; list-style: none; padding-left: 0;">
                        <li style="margin-bottom: 1em;">
                            <strong>Cb言語の紹介</strong>
                            <div style="font-size: 0.85em; color: #e0e0e0; margin-top: 0.3em; margin-left: 1.5em;">
                                どんな言語か、どんな機能があるか
                            </div>
                        </li>
                        <li>
                            <strong>バイブコーディングでの開発</strong>
                            <div style="font-size: 0.85em; color: #e0e0e0; margin-top: 0.3em; margin-left: 1.5em;">
                                AI駆動開発の実践と知見
                            </div>
                        </li>
                    </ul>
                </div>

                <!-- 話さないこと -->
                <div style="background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%); padding: 1.5em; border-radius: 12px; color: white;">
                    <h3 style="color: white; margin-bottom: 1.5em; font-size: 1.3em;">❌ お話ししないこと</h3>
                    <ul style="text-align: left; font-size: 0.95em; line-height: 2; list-style: none; padding-left: 0;">
                        <li>
                            <strong>内部実装の詳細</strong>
                            <div style="font-size: 0.85em; color: #ffe0e0; margin-top: 0.3em; margin-left: 1.5em;">
                                インタプリタ/コンパイラの<br/>アルゴリズムや最適化手法
                            </div>
                        </li>
                    </ul>
                    <div style="margin-top: 2em; padding: 1em; background: rgba(255,255,255,0.2); border-radius: 8px; font-size: 0.85em;">
                        💡 実装に興味がある方は<br/>ぜひ後で質問してください！
                    </div>
                </div>
            </div>
        </div>
    </section>`;
}
