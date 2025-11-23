export default function memoryManagement(): string {
    return `<section class="memory-management-slide">
        <h2>メモリ管理</h2>

        <div class="code-section-large">
            <h3>💾 動的メモリ割り当て</h3>
            <pre><code class="language-cb">// 配列の動的割り当て
int[100]* arr = new int[100];
for (int i = 0; i &lt; 100; i++) {
    arr[i] = i * 2;
}
delete[] arr;</code></pre>
            <pre><code class="language-cb">// 構造体の動的割り当て
struct Data {
    int value;
    string name;
};

Data* data = new Data;
data-&gt;value = 42;
data-&gt;name = "example";
delete data;

// 手動メモリ管理（低レベル）
void* buffer = malloc(1024);
memcpy(buffer, sourceData, size);
free(buffer);
</code></pre>
        </div>

        <div class="feature-note">
            <p>🔧 <strong>C++ライクなメモリ管理機能</strong></p>
        </div>
    </section>`;
}