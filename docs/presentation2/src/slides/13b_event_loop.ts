export default function eventLoop(): string {
    return `<section class="event-loop-slide">
        <h2>イベントループと協調的動作</h2>

        <div class="two-column-layout">
            <div class="column">
                <h3>🔄 イベントループの仕組み</h3>
                <ul>
                    <li><strong>協調的マルチタスク</strong><br>各タスクが自発的にCPUを譲る</li>
                    <li><strong>yield による制御</strong><br>明示的な実行権の譲渡ポイント</li>
                    <li><strong>Auto-yield機能</strong><br>async関数は1処理ごとに自動yield</li>
                    <li><strong>軽量な並行処理</strong><br>OSスレッドを使わない効率的な実装</li>
                </ul>
            </div>

            <div class="column">
                <h3>⏱️ sleep関数の実装</h3>
                <ul>
                    <li><strong>時間経過のみを監視</strong><br>実際にスレッドをブロックしない</li>
                    <li><strong>イベントループで管理</strong><br>タイムアウト時刻を記録して待機</li>
                    <li><strong>他タスクに譲る</strong><br>sleep中は他のタスクが実行可能</li>
                </ul>

                <pre style="font-size: 0.4em; margin-top: 1em;"><code class="language-cb">// sleepの動作イメージ
async void example() {
    println("Start");
    sleep(1000);  // 1秒待機
    // ← ここで他のタスクが実行される
    println("After 1 second");
}</code></pre>
            </div>
        </div>

        <div class="feature-note">
            <p>⚙️ <strong>Auto-yieldにより、明示的なyield不要で協調的動作を実現</strong></p>
        </div>
    </section>`;
}