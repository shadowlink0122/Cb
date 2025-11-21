export default function asyncAwait(): string {
    return `<section class="async-await-slide">
        <h2>Async/Await - 協調的マルチタスク</h2>

        <div class="async-features">
            <div class="async-block">
                <h3>⚡ 非同期関数の定義と使用</h3>
                <pre><code class="language-cb">// 非同期関数の定義
async void task_with_sleep(string name, int duration) {
    println("{name}: Start (will sleep {duration}ms)");
    sleep(duration);  // 非同期スリープ
    println("{name}: After sleep");
    yield;  // 明示的な協調ポイント
    println("{name}: End");
}

// 並行実行の例
async void processMultipleTasks() {
    // 複数のタスクを同時開始
    Future&lt;int&gt; task1 = fetchData("api/users");
    Future&lt;int&gt; task2 = fetchData("api/posts");
    Future&lt;int&gt; task3 = fetchData("api/comments");

    // すべての結果を待つ
    int users = await task1;
    int posts = await task2;
    int comments = await task3;

    println("Total: ", users + posts + comments);
}</code></pre>
            </div>

            <div class="async-block">
                <h3>🔄 イベントループと協調的動作</h3>
                <pre><code class="language-cb">// タイマーとイベント処理
async void animation() {
    for (int frame = 0; frame < 60; frame++) {
        drawFrame(frame);
        await sleep(16);  // 約60FPS
    }
}

// I/O操作の非同期化
async string readFile(string path) {
    FileHandle file = openAsync(path);
    string content = await file.read();
    file.close();
    return content;
}

// エラーハンドリング
async void safeOperation() {
    try {
        int result = await riskyAsyncOperation();
        println("Success: ", result);
    } catch (Error e) {
        println("Failed: ", e.message);
    }
}</code></pre>
            </div>

            <div class="implementation-note">
                <h3>📊 協調的マルチタスクの利点</h3>
                <div class="benefits-grid">
                    <div class="benefit">
                        <strong>予測可能</strong>
                        <p>明示的なyieldポイント</p>
                    </div>
                    <div class="benefit">
                        <strong>軽量</strong>
                        <p>OSスレッド不要</p>
                    </div>
                    <div class="benefit">
                        <strong>安全</strong>
                        <p>データ競合なし</p>
                    </div>
                    <div class="benefit">
                        <strong>効率的</strong>
                        <p>コンテキストスイッチ最小</p>
                    </div>
                </div>
            </div>
        </div>
    </section>`;
}