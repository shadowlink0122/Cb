#!/usr/bin/env python3
"""
最終的なプレゼンテーション修正スクリプト
- ページ下部での見切れ修正
- 全てのCB→Cbの修正
- シンタックスハイライトの完全な適用
- 構文エラーの修正
- ページ分割
"""

import re
import sys

def fix_overflow_pages(html):
    """ページ下部で見切れている部分を修正"""
    
    # ページ37以降の見切れ修正
    # イベントループの実装を2ページに分割
    html = html.replace(
        '''            <!-- スライド37: イベントループの実装 -->
            <section class="left-align">
                <h2>イベントループの実装''',
        '''            <!-- スライド37: イベントループの実装（1/2） -->
            <section class="left-align">
                <h2>イベントループの実装（1/2）'''
    )
    
    # イベントループの実装を2つに分割
    event_loop_pattern = re.compile(
        r'(<!-- スライド37: イベントループの実装\(1/2\) -->.*?</section>)',
        re.DOTALL
    )
    
    match = event_loop_pattern.search(html)
    if match:
        original = match.group(1)
        
        # 1ページ目: タスクキューと基本構造
        page1 = '''            <!-- スライド37: イベントループの実装（1/2） -->
            <section class="left-align">
                <h2>イベントループの実装（1/2）</h2>
                <div style="width: 95%; margin: 0.3em auto 0;">
                    <h3 style="font-size: 0.75em; margin-bottom: 0.3em;">タスクキューと基本構造</h3>
                    <pre style="margin: 0.3em 0;"><code class="language-cpp" style="font-size: 0.85em;">struct Task {
    std::function&lt;void()&gt; func;
    std::string task_id;
    int64_t wake_time_ms = 0;
    bool is_sleeping = false;
};

class EventLoop {
    std::queue&lt;Task&gt; ready_queue;
    std::priority_queue&lt;Task, std::vector&lt;Task&gt;, TaskComparator&gt; sleeping_tasks;
    std::unordered_map&lt;std::string, std::shared_ptr&lt;Future&gt;&gt; futures;
    
public:
    void spawn_task(std::function&lt;void()&gt; task);
    void run();
    void sleep_ms(int64_t duration);
};</code></pre>
                    <ul style="font-size: 0.6em; line-height: 1.3; margin: 0.4em 0 0 0; padding-left: 1.3em;">
                        <li><strong>ready_queue</strong>: 実行可能なタスクのキュー</li>
                        <li><strong>sleeping_tasks</strong>: sleep中のタスク（優先度付きキュー）</li>
                        <li><strong>futures</strong>: 非同期結果の管理</li>
                    </ul>
                </div>
            </section>'''
        
        # 2ページ目: イベントループの実行
        page2 = '''
            <!-- スライド37-2: イベントループの実装（2/2） -->
            <section class="left-align">
                <h2>イベントループの実装（2/2）</h2>
                <div style="width: 95%; margin: 0.3em auto 0;">
                    <h3 style="font-size: 0.75em; margin-bottom: 0.3em;">実行ループとタスク管理</h3>
                    <pre style="margin: 0.3em 0;"><code class="language-cpp" style="font-size: 0.85em;">void EventLoop::run() {
    while (!ready_queue.empty() || !sleeping_tasks.empty()) {
        // スリープ中のタスクをチェック
        int64_t now = current_time_ms();
        while (!sleeping_tasks.empty() && 
               sleeping_tasks.top().wake_time_ms &lt;= now) {
            ready_queue.push(sleeping_tasks.top());
            sleeping_tasks.pop();
        }
        
        // 実行可能なタスクを処理
        if (!ready_queue.empty()) {
            Task task = ready_queue.front();
            ready_queue.pop();
            task.func();  // タスク実行
        } else {
            // 次のタスクまで待機
            if (!sleeping_tasks.empty()) {
                sleep_until(sleeping_tasks.top().wake_time_ms);
            }
        }
    }
}</code></pre>
                    <ul style="font-size: 0.6em; line-height: 1.3; margin: 0.4em 0 0 0; padding-left: 1.3em;">
                        <li><strong>協調的マルチタスク</strong>: タスクは明示的に制御を譲る</li>
                        <li><strong>ノンブロッキング</strong>: sleepは他のタスクをブロックしない</li>
                    </ul>
                </div>
            </section>'''
        
        html = html.replace(original, page1 + page2)
    
    # 非同期処理とsleepの実装を2ページに分割
    async_sleep_pattern = re.compile(
        r'(<!-- スライド\d+: 非同期処理とsleep -->.*?</section>)',
        re.DOTALL
    )
    
    match = async_sleep_pattern.search(html)
    if match:
        original = match.group(1)
        
        page1 = '''            <!-- 非同期処理とsleep（1/2） -->
            <section class="left-align">
                <h2>非同期処理とsleep（1/2）</h2>
                <div style="width: 95%; margin: 0.3em auto 0;">
                    <h3 style="font-size: 0.75em; margin-bottom: 0.3em;">タスクごとの時間管理</h3>
                    <pre style="margin: 0.3em 0;"><code class="language-cb" style="font-size: 0.9em;"><span class="hljs-keyword">async</span> <span class="hljs-type">void</span> task1() {
    println(<span class="hljs-string">"Task 1: Start"</span>);
    <span class="hljs-keyword">await</span> sleep(<span class="hljs-number">1000</span>);  <span class="hljs-comment">// 1秒待機（他のタスクはブロックされない）</span>
    println(<span class="hljs-string">"Task 1: After 1s"</span>);
}

<span class="hljs-keyword">async</span> <span class="hljs-type">void</span> task2() {
    println(<span class="hljs-string">"Task 2: Start"</span>);
    <span class="hljs-keyword">await</span> sleep(<span class="hljs-number">500</span>);   <span class="hljs-comment">// 0.5秒待機</span>
    println(<span class="hljs-string">"Task 2: After 0.5s"</span>);
}

<span class="hljs-comment">// 実行順序: Task 1 Start → Task 2 Start → Task 2 After 0.5s → Task 1 After 1s</span></code></pre>
                    <ul style="font-size: 0.6em; line-height: 1.3; margin: 0.4em 0 0 0; padding-left: 1.3em;">
                        <li><strong>ノンブロッキング</strong>: sleepは他のタスクをブロックしない</li>
                        <li><strong>タスクごとの時間経過</strong>: 各タスクが独立して時間を管理</li>
                    </ul>
                </div>
            </section>'''
        
        page2 = '''
            <!-- 非同期処理とsleep（2/2） -->
            <section class="left-align">
                <h2>非同期処理とsleep（2/2）</h2>
                <div style="width: 95%; margin: 0.3em auto 0;">
                    <h3 style="font-size: 0.75em; margin-bottom: 0.3em;">コンカレントとシーケンシャル実行</h3>
                    <div style="display: flex; gap: 1em;">
                        <div style="flex: 1;">
                            <h4 style="font-size: 0.7em; margin-bottom: 0.3em;">コンカレント実行</h4>
                            <pre style="margin: 0.3em 0;"><code class="language-cb" style="font-size: 0.85em;"><span class="hljs-keyword">async</span> <span class="hljs-type">void</span> main() {
    <span class="hljs-comment">// 並行実行（待たない）</span>
    spawn task1();
    spawn task2();
    <span class="hljs-comment">// task1とtask2は並行実行</span>
}</code></pre>
                            <p style="font-size: 0.6em; margin: 0.3em 0;">実行時間: max(task1, task2)</p>
                        </div>
                        <div style="flex: 1;">
                            <h4 style="font-size: 0.7em; margin-bottom: 0.3em;">シーケンシャル実行</h4>
                            <pre style="margin: 0.3em 0;"><code class="language-cb" style="font-size: 0.85em;"><span class="hljs-keyword">async</span> <span class="hljs-type">void</span> main() {
    <span class="hljs-comment">// 順次実行（待つ）</span>
    <span class="hljs-keyword">await</span> task1();
    <span class="hljs-keyword">await</span> task2();
    <span class="hljs-comment">// task1完了後にtask2実行</span>
}</code></pre>
                            <p style="font-size: 0.6em; margin: 0.3em 0;">実行時間: task1 + task2</p>
                        </div>
                    </div>
                    <ul style="font-size: 0.6em; line-height: 1.3; margin: 0.4em 0 0 0; padding-left: 1.3em;">
                        <li><strong>spawn</strong>: タスクを起動して即座に戻る（並行実行）</li>
                        <li><strong>await</strong>: タスクの完了を待つ（順次実行）</li>
                    </ul>
                </div>
            </section>'''
        
        html = html.replace(original, page1 + page2)
    
    return html

def fix_all_syntax_highlighting(html):
    """全てのコードブロックにシンタックスハイライトを適用"""
    
    # async, await, match, interface, impl を赤系統に
    replacements = [
        # キーワードのハイライト修正
        (r'<code>(async)', r'<code><span class="hljs-keyword">async</span>'),
        (r'<code>([^<]*?)(async)([^<]*?)</code>', 
         lambda m: f'<code>{m.group(1)}<span class="hljs-keyword">async</span>{m.group(3)}</code>'),
        
        # C++のasyncハイライト
        (r'std::async', r'std::<span class="hljs-keyword">async</span>'),
        
        # メソッド呼び出しのハイライト
        (r'\.([a-zA-Z_][a-zA-Z0-9_]*)\(', r'.<span class="hljs-function">\1</span>('),
        (r'\.([a-zA-Z_][a-zA-Z0-9_]*)', r'.<span class="hljs-property">\1</span>'),
    ]
    
    for pattern, replacement in replacements:
        html = re.sub(pattern, replacement, html)
    
    return html

def fix_cb_naming(html):
    """全てのCB→Cbに修正"""
    # タイトルやコンテンツ内のCBをCbに変更
    # ただし、HTMLタグ内は除外
    html = re.sub(r'>CB<', r'>Cb<', html)
    html = re.sub(r'>CB(?=[^<>]*<)', r'>Cb', html)
    html = re.sub(r'CB インタプリタ', r'Cb インタプリタ', html)
    html = re.sub(r'CB の', r'Cb の', html)
    html = re.sub(r'CB を', r'Cb を', html)
    html = re.sub(r'CB は', r'Cb は', html)
    html = re.sub(r'CB が', r'Cb が', html)
    html = re.sub(r'言語CB', r'言語Cb', html)
    
    return html

def fix_date(html):
    """日付を11/21に修正"""
    html = re.sub(r'2024/11/\d+', r'2024/11/21', html)
    html = re.sub(r'2025年7月', r'2025年7月', html)
    html = re.sub(r'2024年', r'2025年', html, flags=re.MULTILINE)
    
    return html

def main():
    input_file = '/Users/shadowlink/Documents/git/Cb/presentation/cb_interpreter_presentation.html'
    
    with open(input_file, 'r', encoding='utf-8') as f:
        html = f.read()
    
    print("修正を適用中...")
    
    # 1. CB→Cb修正
    html = fix_cb_naming(html)
    print("✓ CB→Cb修正完了")
    
    # 2. 日付修正
    html = fix_date(html)
    print("✓ 日付修正完了")
    
    # 3. ページオーバーフロー修正
    html = fix_overflow_pages(html)
    print("✓ ページオーバーフロー修正完了")
    
    # 4. シンタックスハイライト修正
    html = fix_all_syntax_highlighting(html)
    print("✓ シンタックスハイライト修正完了")
    
    # バックアップ作成
    backup_file = input_file + '.backup_final'
    with open(backup_file, 'w', encoding='utf-8') as f:
        f.write(open(input_file, 'r', encoding='utf-8').read())
    print(f"✓ バックアップ作成: {backup_file}")
    
    # 修正版を保存
    with open(input_file, 'w', encoding='utf-8') as f:
        f.write(html)
    
    print(f"\n✅ 修正完了: {input_file}")
    print("\n次のコマンドでプレゼンテーションを確認できます:")
    print("  cd /Users/shadowlink/Documents/git/Cb/presentation")
    print("  python3 -m http.server 8000")
    print("  ブラウザで http://localhost:8000/cb_interpreter_presentation.html を開く")

if __name__ == '__main__':
    main()
