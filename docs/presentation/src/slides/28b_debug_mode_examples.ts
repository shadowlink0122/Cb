export default function debugModeExamples(): string {
    return `<section class="debug-mode-examples-slide">
        <h2>デバッグモードの実例</h2>

        <div class="code-section">
            <h3>📝 コード内でのデバッグ出力</h3>
            <pre><code class="language-cpp">#ifdef DEBUG_MODE
    std::cerr &lt;&lt; "[DEBUG] Parsing function: "
              &lt;&lt; func_name &lt;&lt; std::endl;
    std::cerr &lt;&lt; "[DEBUG] Parameter count: "
              &lt;&lt; params.size() &lt;&lt; std::endl;
#endif</code></pre>
        </div>

        <div class="code-section">
            <h3>🖥️ 実行例</h3>
            <pre><code class="language-bash"># 通常実行
$ ./cb compile test.cb
Compilation successful

# デバッグモード実行
$ ./cb --debug compile test.cb
[DEBUG] Parsing function: main
[DEBUG] Parameter count: 0
[DEBUG] Entering HIR conversion
[DEBUG] Converting expression: BinaryOp
[DEBUG] Left: IntLiteral(10)
[DEBUG] Right: IntLiteral(20)
Compilation successful</code></pre>
        </div>

        <div class="feature-note">
            <p>🎯 <strong>問題箇所が一目瞭然</strong></p>
        </div>
    </section>`;
}