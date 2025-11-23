export default function basicSyntax(): string {
    return `<section class="basic-syntax-slide">
        <h2>基本構文 - C++がベース</h2>

        <div class="syntax-comparison">
            <div class="code-section">
                <h3>📝 変数宣言と基本型</h3>
                <pre><code class="language-cb">// Cbの基本構文はC++と同じ
int x = 42;
string name = "Cb Lang";
bool flag = true;
double pi = 3.14159;

// ポインタと参照
int* ptr = &x;
int& ref = x;

// 配列
int[10] numbers;
int[3][3] matrix;

// constとstatic
const int MAX = 100;
static int counter = 0;</code></pre>
            </div>

            <div class="code-section">
                <h3>🔄 制御構造</h3>
                <pre><code class="language-cb">// if-else文
if (x > 0) {
    println("Positive");
} else if (x < 0) {
    println("Negative");
} else {
    println("Zero");
}

// ループ構造
for (int i = 0; i < 10; i++) {
    println(i);
}

while (condition) {
    // 処理
}

// switch文
switch (value) {
    case(1) {
        println("One");
    }
    case(2 | 3) {
        println("Two or Three");
    }
    else {
        println("Other");
    }
}</code></pre>
            </div>
        </div>

        <div class="feature-note">
            <p>💡 <strong>C++プログラマならすぐに書ける</strong> - 学習コストゼロで開始可能</p>
        </div>
    </section>`;
}