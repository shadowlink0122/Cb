export default function basicTypes(): string {
    return `<section class="basic-types-slide">
        <h2>基本構文 - 変数と基本型</h2>

        <div class="code-section-large">
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

        <div class="feature-note">
            <p>💡 <strong>C++プログラマならすぐに書ける</strong> - 学習コストゼロで開始可能</p>
            <p>💡 <strong>ポインタ(*T)と参照(&T)</strong> - 低レベル操作もサポート</p>
            <p>💡 <strong>配列型</strong> - 配列情報は型に付属する</p>
        </div>
    </section>`;
}