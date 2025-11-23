export default function testingExamples(): string {
    return `<section class="testing-examples-slide">
        <h2>テストの実例</h2>

        <div class="code-section-large">
            <h3>📝 簡単なテストケース</h3>
            <pre><code class="language-cb">// tests/cases/basic/test_arithmetic.cb
int main() {
    int a = 10;
    int b = 20;
    int sum = a + b;
    assert(sum == 30);  // 期待値チェック
    return 0;
}</code></pre>
        </div>

        <div class="code-section-large">
            <h3>🧪 テスト実行スクリプト</h3>
            <pre><code class="language-bash"># tests/integration/run_unified_tests.sh
for test in tests/cases/**/*.cb; do
    echo "Testing: $test"
    ./cb compile $test -o /tmp/test_out
    if [ $? -eq 0 ]; then
        /tmp/test_out  # 実行して結果確認
        echo "✅ PASS"
    else
        echo "❌ FAIL: Compilation error"
    fi
done</code></pre>
        </div>

        <div class="feature-note">
            <p>⚡ <strong>200+ テストが数秒で完了</strong></p>
        </div>
    </section>`;
}