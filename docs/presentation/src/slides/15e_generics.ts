export default function generics(): string {
    return `<section class="generics-slide">
        <h2>ジェネリクス</h2>

        <div class="code-section-large">
            <h3>📝 型パラメータによる汎用プログラミング</h3>
            <pre><code class="language-cb">// ジェネリック構造体
struct Stack< T > {
    T[100] items;
    int top;

    void push(T item) {
        items[top++] = item;
    }

    T pop() {
        return items[--top];
    }
};

// 使用例
Stack&lt;int&gt; intStack;
intStack.push(42);
int value = intStack.pop();</code></pre>
        </div>

        <div class="feature-note">
            <p>🔮 <strong>C++スタイルのテンプレートシステム</strong></p>
        </div>
    </section>`;
}