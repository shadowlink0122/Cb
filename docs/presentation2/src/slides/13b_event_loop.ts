export default function eventLoop(): string {
    return `<section class="event-loop-slide">
        <h2>イベントループと協調的動作</h2>

        <div class="code-section-large">
            <h3>🔄 非同期処理パターン</h3>
            <pre><code class="language-cb">// タイマーとイベント処理
async void animation() {
    for (int frame = 0; frame < 60; frame++) {
        drawFrame(frame);
        await sleep(16);  // 約60FPS
    }
}

// I/O操作の非同期化
async Result&lt;string, IOError&gt; readFile(string path) {
    FileHandle file = openAsync(path);
    if (!file.isValid()) {
        return Result&lt;string, IOError&gt;::Err(IOError("Failed to open"));
    }
    string content = await file.read();
    file.close();
    return Result&lt;string, IOError&gt;::Ok(content);
}

// Result型でエラーハンドリング
async void processData() {
    Result&lt;int, string&gt; result = await fetchData();
    match (result) {
        Ok(value) => { println("Success: ", value); }
        Err(error) => { println("Failed: ", error); }
    }
}</code></pre>
        </div>

        <div class="feature-note">
            <p>⚙️ <strong>OSスレッドを使わない軽量な並行処理</strong></p>
        </div>
    </section>`;
}