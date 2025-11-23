export default function constructorDestructor(): string {
    return `<section class="constructor-destructor-slide">
        <h2>コンストラクタ/デストラクタ & defer</h2>

        <div class="two-column-layout">
            <div class="column">
                <h3>🔨 コンストラクタ/デストラクタ</h3>
                <pre style="font-size: 0.45em;"><code class="language-cb">struct FileHandle {
    private int fd;
    private string path;

    // コンストラクタ
    constructor(string filepath) {
        this.path = filepath;
        this.fd = open(filepath);
        println("File opened: {filepath}");
    }

    // デストラクタ
    destructor() {
        if (this.fd >= 0) {
            close(this.fd);
            println("File closed: {this.path}");
        }
    }
}

void example() {
    FileHandle file("data.txt");
    // 使用...
}  // スコープを抜けると自動でデストラクタ呼び出し</code></pre>
            </div>

            <div class="column">
                <h3>🔄 defer - 遅延実行</h3>
                <pre style="font-size: 0.45em;"><code class="language-cb">void processFile(string path) {
    int fd = open(path);
    defer close(fd);  // 関数終了時に自動実行

    if (!isValid(fd)) {
        return;  // ここでもclose()が呼ばれる
    }

    string data = read(fd);
    defer println("Processing complete");

    processData(data);
    // 関数終了時:
    // 1. println("Processing complete")
    // 2. close(fd)  (逆順で実行)
}

// Go言語風のリソース管理
void multipleDefer() {
    defer println("3: Last");
    defer println("2: Middle");
    defer println("1: First");
    // 実行順: 1 → 2 → 3 (LIFO)
}</code></pre>
                <ul style="font-size: 0.75em; margin-top: 1em;">
                    <li><strong>LIFO順で実行</strong><br>登録と逆順でクリーンアップ</li>
                    <li><strong>例外安全</strong><br>どんな終了経路でも実行</li>
                </ul>
            </div>
        </div>

        <div class="feature-note">
            <p>🛡️ <strong>RAII + defer で確実なリソース管理を実現</strong></p>
        </div>
    </section>`;
}
