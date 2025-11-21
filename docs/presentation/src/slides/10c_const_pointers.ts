export default function constPointers(): string {
    return `<section class="const-pointers-slide">
        <h2>const修飾子とポインタ</h2>

        <div class="code-section-large">
            <h3>🔒 constの位置による意味の違い</h3>
            <pre><code class="language-cb">// 1. 値が固定（ポインタが指す値を変更不可）
const int* ptr1 = &value;
*ptr1 = 10;        // ❌ エラー：値の変更不可
ptr1 = &other;     // ✅ OK：ポインタ自体は変更可能

// 2. アドレスが固定（ポインタ自体を変更不可）
int* const ptr2 = &value;
*ptr2 = 10;        // ✅ OK：値の変更可能
ptr2 = &other;     // ❌ エラー：ポインタの変更不可

// 3. 両方固定（値もアドレスも変更不可）
const int* const ptr3 = &value;
*ptr3 = 10;        // ❌ エラー：値の変更不可
ptr3 = &other;     // ❌ エラー：ポインタの変更不可

// 実用例：読み取り専用配列の参照
void printArray(const int* const arr, int size) {
    // arrが指す配列の内容もarrのアドレスも変更不可
    // 安全に配列を読み取り専用で処理
    for (int i = 0; i < size; i++) {
        println(arr[i]);
    }
}</code></pre>
        </div>

        <div class="feature-note">
            <p>💡 <strong>メモリ安全性：constで意図しない変更を防止</strong></p>
        </div>
    </section>`;
}