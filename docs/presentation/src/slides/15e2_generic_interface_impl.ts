export default function genericInterfaceImpl(): string {
    return `<section class="generic-interface-impl-slide">
        <h2>ジェネリクス - Interface/Impl</h2>

        <div class="code-section-large">
            <h3>🔮 ジェネリックなインターフェース実装</h3>
            <pre><code class="language-cb">// ジェネリックインターフェース
interface Container&lt;T&gt; {
    void add(T item);
    T get(int index);
    int size();
};

// ジェネリック構造体
struct Box&lt;T&gt; {
    T[100] items;
    int count;
};

// ジェネリックなImpl
impl Container&lt;T&gt; for Box&lt;T&gt; {
    void add(T item) {
        this.items[this.count++] = item;
    }

    T get(int index) {
        return this.items[index];
    }

    int size() {
        return this.count;
    }
};

// 使用例
Box&lt;int&gt; intBox;
intBox.add(42);
intBox.add(100);
int val = intBox.get(0);  // 42</code></pre>
        </div>

        <div class="feature-note">
            <p>🎯 <strong>impl I&lt;T&gt; for S&lt;T&gt; - 型パラメータを保持したまま実装</strong></p>
        </div>
    </section>`;
}
