export default function asyncBasics(): string {
    return `<section class="async-basics-slide">
        <h2>Async/Await - 非同期関数の基本</h2>

        <div class="code-section-large">
            <h3>⚡ 非同期関数の定義</h3>
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

        <div class="feature-note">
            <p>🔄 <strong>協調的マルチタスクで効率的な並行処理</strong></p>
        </div>
    </section>`;
}