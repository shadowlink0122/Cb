export default function vscodeSyntax(): string {
    return `<section class="vscode-syntax-slide">
        <div style="display: flex; align-items: center; justify-content: center; gap: 1em; margin-bottom: 1em;">
            <img src="/assets/cb-logo.png" alt="Cb Logo" style="width: 60px; height: 60px;" />
            <h2 style="margin: 0;">開発環境サポート - VSCode拡張</h2>
        </div>

        <div class="two-column-layout">
            <div class="column">
                <h3>🎨 シンタックスハイライト</h3>
                <ul>
                    <li><strong>TextMate文法ファイル</strong><br>VSCode用のシンタックス定義を実装</li>
                    <li><strong>キーワードのハイライト</strong><br>async, match, impl等の強調表示</li>
                    <li><strong>型の色分け</strong><br>Option, Result, Future等</li>
                    <li><strong>コメントと文字列</strong><br>適切な色分けで可読性向上</li>
                </ul>

                <div style="margin-top: 1em; padding: 1em; background: #f8f9fa; border-radius: 6px;">
                    <p style="font-size: 0.8em; margin: 0;">
                        <strong>📦 拡張機能：</strong><br>
                        <code style="font-size: 0.9em;">.vscode/extensions/cb-language</code>
                    </p>
                </div>
            </div>

            <div class="column">
                <h3>✨ サポート内容</h3>
                <ul>
                    <li><strong>ファイル認識</strong><br>.cb 拡張子を自動認識</li>
                    <li><strong>括弧マッチング</strong><br>対応する括弧をハイライト</li>
                    <li><strong>コメント切り替え</strong><br>Cmd/Ctrl + / でコメント化</li>
                    <li><strong>インデント</strong><br>自動インデント機能</li>
                </ul>

                <div style="margin-top: 1em; padding: 1em; background: #e8f5e9; border-radius: 6px; border-left: 4px solid #4caf50;">
                    <p style="font-size: 0.85em; margin: 0; line-height: 1.5;">
                        <strong>💡 今後の展開</strong><br>
                        LSP（Language Server Protocol）を実装し、<br>
                        補完、定義ジャンプ、エラー検出等の<br>
                        高度な機能を提供予定
                    </p>
                </div>
            </div>
        </div>

        <div class="feature-note">
            <p>🛠️ <strong>快適な開発体験のために、基本的なエディタサポートを実装</strong></p>
        </div>
    </section>`;
}
