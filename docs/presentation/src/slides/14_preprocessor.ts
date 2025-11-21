export default function preprocessor(): string {
    return `<section class="preprocessor-slide">
        <h2>プリプロセッサとマクロ</h2>

        <div class="preprocessor-features">
            <div class="feature-column">
                <h3>📁 インクルードとモジュール</h3>
                <pre><code class="language-cb">// ヘッダファイルのインクルード
#include "math.cb"
#include "utils.cb"

// 標準ライブラリ
#include <string.cb>
#include <vector.cb>

// インクルードガード
#ifndef MYHEADER_CB
#define MYHEADER_CB
    // ヘッダの内容
#endif

// 条件付きコンパイル
#ifdef DEBUG
    #define LOG(msg) println("[DEBUG] ", msg)
#else
    #define LOG(msg)  // 空のマクロ
#endif</code></pre>
            </div>

            <div class="feature-column">
                <h3>🔧 マクロ定義と展開</h3>
                <pre><code class="language-cb">// 単純なマクロ
#define PI 3.14159265359
#define MAX_SIZE 1024

// 関数マクロ
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// 複雑なマクロ
#define FOR_EACH(item, array, size) \\
    for (int _i = 0; _i < (size); _i++) { \\
        auto item = (array)[_i];

#define END_FOR_EACH }

// 使用例
int numbers[] = {1, 2, 3, 4, 5};
FOR_EACH(num, numbers, 5)
    println(num * 2);
END_FOR_EACH</code></pre>
            </div>

            <div class="feature-column">
                <h3>🎯 高度なマクロ技法</h3>
                <pre><code class="language-cb">// 文字列化マクロ
#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)

// トークン連結
#define CONCAT(a, b) a##b
#define MAKE_FUNC(name) void func_##name()

// 可変長マクロ
#define DEBUG_PRINT(fmt, ...) \\
    printf("[%s:%d] " fmt "\\n", \\
           __FILE__, __LINE__, ##__VA_ARGS__)

// コンパイル時アサート
#define STATIC_ASSERT(cond) \\
    typedef char static_assert_##__LINE__[(cond) ? 1 : -1]

// 使用例
DEBUG_PRINT("Value: %d", 42);
// 出力: [file.cb:123] Value: 42</code></pre>
            </div>
        </div>

        <div class="preprocessor-note">
            <p>⚠️ <strong>C/C++互換のプリプロセッサ</strong> - 既存のヘッダファイルも利用可能</p>
        </div>
    </section>`;
}