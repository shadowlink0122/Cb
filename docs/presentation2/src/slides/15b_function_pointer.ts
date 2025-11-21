export default function functionPointer(): string {
    return `<section class="function-pointer-slide">
        <h2>関数ポインタ</h2>

        <div class="code-section-large">
            <h3>🔗 高階関数とコールバック</h3>
            <pre><code class="language-cb">// 関数ポインタ型定義（戻り値の型のみ）
typedef int* Operation;

int add(int a, int b) { return a + b; }
int mul(int a, int b) { return a * b; }

// 関数ポインタの使用
Operation op = &add;
int result = op(5, 3);  // 8

// 関数を返す関数
Operation getOp(char symbol) {
    if (symbol == '+') return &add;
    if (symbol == '*') return &mul;
    return nullptr;
}

// 高階関数の例
void apply(Operation op, int x, int y) {
    println("Result: ", op(x, y));
}

apply(&add, 10, 20);  // Result: 30
apply(&mul, 5, 7);    // Result: 35</code></pre>
        </div>

        <div class="feature-note">
            <p>⚡ <strong>動的な関数選択とコールバック機構</strong></p>
        </div>
    </section>`;
}