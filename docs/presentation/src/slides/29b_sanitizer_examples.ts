export default function sanitizerExamples(): string {
    return `<section class="sanitizer-examples-slide">
        <h2>サニタイザーの実例</h2>

        <div class="code-section">
            <h3>🔧 Makefileでの設定</h3>
            <pre><code class="language-makefile"># AddressSanitizer有効化
CXXFLAGS += -fsanitize=address
CXXFLAGS += -fno-omit-frame-pointer
LDFLAGS += -fsanitize=address

# UndefinedBehaviorSanitizer
CXXFLAGS += -fsanitize=undefined</code></pre>
        </div>

        <div class="code-section">
            <h3>🐛 検出例: メモリリーク</h3>
            <pre><code class="language-text">==12345==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 100 bytes in 1 object(s) allocated from:
    #0 in operator new(unsigned long)
    #1 in Parser::parseExpression() parser.cpp:234
    #2 in Parser::parseStatement() parser.cpp:156

SUMMARY: AddressSanitizer: 100 bytes leaked in 1 allocation(s)</code></pre>
        </div>

        <div class="feature-note">
            <p>⚡ <strong>問題の行番号まで正確に教えてくれる</strong></p>
        </div>
    </section>`;
}