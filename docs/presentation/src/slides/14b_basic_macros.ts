export default function basicMacros(): string {
    return `<section class="basic-macros-slide">
        <h2>プリプロセッサ - 基本的なマクロ</h2>

        <div class="code-section-large">
            <h3>🔧 マクロ定義と展開</h3>
            <pre><code class="language-cb">// 単純なマクロ
#define PI 3.14159265359
#define MAX_SIZE 1024

// 関数マクロ
#define MIN(a, b) ((a) &lt; (b) ? (a) : (b))
#define MAX(a, b) ((a) &gt; (b) ? (a) : (b))

// 複雑なマクロ
#define FOR_EACH(item, array, size) \\
    for (int _i = 0; _i &lt; (size); _i++) { \\
        auto item = (array)[_i];

#define END_FOR_EACH }

// 使用例
int[5] numbers = {1, 2, 3, 4, 5};
FOR_EACH(num, numbers, 5)
    println(num * 2);
END_FOR_EACH</code></pre>
        </div>

        <div class="feature-note">
            <p>⚙️ <strong>定数定義と簡単なコード生成</strong></p>
        </div>
    </section>`;
}