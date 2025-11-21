export default function advancedFeatures(): string {
    return `<section class="advanced-features-slide">
        <h2>その他の高度な機能</h2>

        <div class="advanced-grid">
            <div class="feature-card">
                <h3>🎲 Enum（列挙型）</h3>
                <pre><code class="language-cb">enum Status {
    OK = 200,
    NOT_FOUND = 404,
    ERROR = 500
}

enum Color { RED, GREEN, BLUE }

Status code = Status::OK;
if (code == Status::NOT_FOUND) {
    println("Not found");
}</code></pre>
            </div>

            <div class="feature-card">
                <h3>🔗 関数ポインタ</h3>
                <pre><code class="language-cb">// 関数ポインタ型定義
typedef Operation = int (*)(int, int);

int add(int a, int b) { return a + b; }
int mul(int a, int b) { return a * b; }

Operation op = &add;
int result = op(5, 3);  // 8

// 関数を返す関数
Operation getOp(char symbol) {
    if (symbol == '+') return &add;
    if (symbol == '*') return &mul;
    return nullptr;
}</code></pre>
            </div>

            <div class="feature-card">
                <h3>💾 メモリ管理</h3>
                <pre><code class="language-cb">// 動的メモリ割り当て
int[100]* arr = new int[100];
delete[] arr;

// スマートポインタ（計画中）
unique_ptr&lt;Object&gt; obj =
    make_unique&lt;Object&gt;();

// 手動メモリ管理
void* buffer = malloc(1024);
memcpy(buffer, data, size);
free(buffer);</code></pre>
            </div>

            <div class="feature-card">
                <h3>🔍 パターンマッチング</h3>
                <pre><code class="language-cb">// switch文の高度な機能
int processCode(int code) {
    switch (code) {
        case(1 | 2 | 3) {
            return code * 10;  // OR条件
        }
        case(10...20) {
            return code * 5;   // 範囲マッチング
        }
        else {
            return -1;         // デフォルト
        }
    }
}

// 複合条件も可能
switch (x) {
    case(5 | 10...15 | 20) {
        println("5, 10-15, 20");
    }
    else {
        println("Other");
    }
}

// match文（Option/Result用）
match (result) {
    Ok(value) => { println(value); }
    Err(error) => { println(error); }
}</code></pre>
            </div>

            <div class="feature-card">
                <h3>📝 ジェネリクス（実装中）</h3>
                <pre><code class="language-cb">// ジェネリック関数
template&lt;typename T&gt;
T max(T a, T b) {
    return a > b ? a : b;
}

// ジェネリック構造体
template&lt;typename T&gt;
struct Stack {
    T[100] items;
    int top;
    void push(T item);
    T pop();
};</code></pre>
            </div>

            <div class="feature-card">
                <h3>🌐 FFI（外部関数インターフェース）</h3>
                <pre><code class="language-cb">// C/C++ライブラリの利用
foreign module "libmath" {
    double sin(double x);
    double cos(double x);
}

// 外部関数の呼び出し
double angle = 1.57;
double y = sin(angle);</code></pre>
            </div>
        </div>
    </section>`;
}