export default function conclusion(): string {
    return `<section class="conclusion-slide">
        <h2>今後の展望</h2>

        <div class="two-column-layout">
            <div class="column">
                <h3>🚀 短期目標（v0.14.0〜v0.16.0）</h3>
                <ul>
                    <li><strong>パターンマッチングの完成</strong><br>より複雑なパターンに対応</li>
                    <li><strong>async/awaitの更なる実装</strong><br>非同期処理の実用化</li>
                    <li><strong>標準ライブラリの拡充</strong><br>ファイルI/O、ネットワーク等</li>
                    <li><strong>エラーメッセージの改善</strong><br>分かりやすいエラー表示</li>
                </ul>
            </div>

            <div class="column">
                <h3>🌟 長期目標（v1.0.0以降）</h3>
                <ul>
                    <li><strong>WebAssembly対応</strong><br>ブラウザでの実行を実現</li>
                    <li><strong>LSP（Language Server）開発</strong><br>VSCode等でのサポート</li>
                    <li><strong>パッケージマネージャ</strong><br>ライブラリの管理システム</li>
                    <li><strong>コミュニティ形成</strong><br>OSS化してユーザーを増やす</li>
                </ul>
            </div>
        </div>

        <div class="feature-note" style="margin-top: 2em;">
            <p>💡 <strong>理想の言語を目指して、継続的に進化させていきます</strong></p>
        </div>
    </section>`;
}
