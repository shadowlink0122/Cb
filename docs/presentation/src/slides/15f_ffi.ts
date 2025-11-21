export default function ffi(): string {
    return `<section class="ffi-slide">
        <h2>FFI（外部関数インターフェース）</h2>

        <div class="code-section-large">
            <h3>🌐 C/C++ライブラリとの連携</h3>
            <pre><code class="language-cb">// C/C++ライブラリの利用
use foreign.math {
    double sin(double x);
    double cos(double x);
    double sqrt(double x);
}

// 外部関数の呼び出し
double angle = 1.57;
double y = sin(angle);
double x = cos(angle);

// システムライブラリの利用
use foreign.c {
    void* malloc(int size);
    void free(void* ptr);
    int printf(string fmt, ...);
}

// C標準ライブラリの活用
void* buffer = malloc(1024);
free(buffer);</code></pre>
        </div>

        <div class="feature-note">
            <p>🔗 <strong>膨大なC/C++エコシステムへのアクセス</strong></p>
        </div>
    </section>`;
}