#!/usr/bin/env python3
"""
Section 3のasync/awaitサンプルを3種類に分割:
1. awaitでタスク待ち
2. awaitしない同時実行
3. エラーハンドリングまたはawait結果の受け取り
"""

import re

def update_async_samples(html_content):
    """Section 3のasync/awaitサンプルを3種類に分割"""
    
    # 既存のasync/awaitセクションを探す
    # Section 3の async/await の既存スライドを探して分割する
    
    # パターン1: awaitでタスク待ち
    await_task_slide = '''
        <section class="left-align">
            <h2>async/await - タスクの待機</h2>
            <h3>awaitでタスクの完了を待つ</h3>
            <pre><code class="language-cb"><span class="hljs-comment">// awaitでタスクを順次実行</span>
<span class="hljs-keyword">async</span> <span class="hljs-type">string</span> <span class="hljs-title function_">fetch_data</span>(<span class="hljs-type">string</span> url) {
    <span class="hljs-keyword">return</span> <span class="hljs-string">"data from "</span> + url;
}

<span class="hljs-keyword">async</span> <span class="hljs-type">void</span> <span class="hljs-title function_">process</span>() {
    <span class="hljs-comment">// 各タスクが完了するまで待機</span>
    <span class="hljs-type">string</span> result1 = <span class="hljs-keyword">await</span> fetch_data(<span class="hljs-string">"api/users"</span>);
    println(result1);
    
    <span class="hljs-type">string</span> result2 = <span class="hljs-keyword">await</span> fetch_data(<span class="hljs-string">"api/posts"</span>);
    println(result2);
    
    <span class="hljs-comment">// 合計実行時間: task1 + task2（順次実行）</span>
}</code></pre>
            <ul>
                <li>awaitで各タスクが完了するまで待機</li>
                <li>順次実行: 次のタスクは前のタスク完了後に開始</li>
                <li>制御フローが分かりやすく、デバッグしやすい</li>
            </ul>
        </section>
'''

    # パターン2: awaitしない同時実行
    concurrent_slide = '''
        <section class="left-align">
            <h2>async/await - 同時実行</h2>
            <h3>awaitしないタスクの並行実行</h3>
            <pre><code class="language-cb"><span class="hljs-comment">// 複数タスクを同時に実行</span>
<span class="hljs-keyword">async</span> <span class="hljs-type">void</span> <span class="hljs-title function_">parallel_process</span>() {
    <span class="hljs-comment">// タスクを起動（awaitしない）</span>
    <span class="hljs-keyword">async</span> fetch_data(<span class="hljs-string">"api/users"</span>);
    <span class="hljs-keyword">async</span> fetch_data(<span class="hljs-string">"api/posts"</span>);
    <span class="hljs-keyword">async</span> fetch_data(<span class="hljs-string">"api/comments"</span>);
    
    <span class="hljs-comment">// 全タスクが並行実行される</span>
    <span class="hljs-comment">// 実行時間: max(task1, task2, task3)</span>
}

<span class="hljs-keyword">async</span> <span class="hljs-type">void</span> <span class="hljs-title function_">mixed_execution</span>() {
    <span class="hljs-comment">// 起動だけして待たない</span>
    <span class="hljs-keyword">async</span> background_task();
    
    <span class="hljs-comment">// 他の処理を続行</span>
    println(<span class="hljs-string">"Background task running..."</span>);
}</code></pre>
            <ul>
                <li>awaitしないタスクは並行実行される</li>
                <li>合計実行時間は最も遅いタスクで決まる</li>
                <li>I/O待ちが多い処理で効果的</li>
            </ul>
        </section>
'''

    # パターン3: エラーハンドリング/結果の受け取り
    error_handling_slide = '''
        <section class="left-align">
            <h2>async/await - 結果の処理</h2>
            <h3>Result型とawaitによるエラーハンドリング</h3>
            <pre><code class="language-cb"><span class="hljs-comment">// Result型を返す非同期関数</span>
<span class="hljs-keyword">async</span> Result&lt;<span class="hljs-type">string</span>, <span class="hljs-type">string</span>&gt; <span class="hljs-title function_">fetch</span>(<span class="hljs-type">string</span> url) {
    <span class="hljs-keyword">if</span> (url == <span class="hljs-string">""</span>) {
        <span class="hljs-keyword">return</span> Err(<span class="hljs-string">"Empty URL"</span>);
    }
    <span class="hljs-keyword">return</span> Ok(<span class="hljs-string">"Success"</span>);
}

<span class="hljs-keyword">async</span> <span class="hljs-type">void</span> <span class="hljs-title function_">handle_result</span>() {
    <span class="hljs-comment">// awaitで結果を受け取る</span>
    Result&lt;<span class="hljs-type">string</span>, <span class="hljs-type">string</span>&gt; result = <span class="hljs-keyword">await</span> fetch(<span class="hljs-string">"api/data"</span>);
    
    <span class="hljs-comment">// パターンマッチで処理</span>
    <span class="hljs-keyword">match</span> (result) {
        Ok(data) =&gt; println(<span class="hljs-string">"Got: "</span> + data),
        Err(e) =&gt; println(<span class="hljs-string">"Error: "</span> + e)
    }
}</code></pre>
            <ul>
                <li>async関数は自動的に非同期タスクとして実行</li>
                <li>awaitで結果を受け取り、エラー処理可能</li>
                <li>Result型とmatchでエラーを安全に処理</li>
            </ul>
        </section>
'''
    
    # Section 3の「型安全な非同期処理」または「Result+async/await」を探して置き換える
    # 重複しているスライドを削除して、3つの新しいスライドに置き換える
    
    # まず既存の重複スライドを探して削除
    patterns_to_remove = [
        r'<section class="left-align">.*?<h2>型安全な非同期処理</h2>.*?</section>',
        r'<section class="left-align">.*?<h2>Result型\+async/await</h2>.*?</section>',
    ]
    
    for pattern in patterns_to_remove:
        html_content = re.sub(pattern, '', html_content, flags=re.DOTALL)
    
    # Section 3の最後の部分（「コミュニティと貢献」の前）に新しいスライドを挿入
    # まず「コミュニティ」セクションを探す
    community_pattern = r'(<section class="left-align">.*?<h2>コミュニティと貢献.*?</h2>)'
    
    new_slides = await_task_slide + '\n' + concurrent_slide + '\n' + error_handling_slide + '\n'
    
    html_content = re.sub(
        community_pattern,
        new_slides + r'\1',
        html_content,
        flags=re.DOTALL
    )
    
    return html_content

if __name__ == "__main__":
    import sys
    
    file_path = "/Users/shadowlink/Documents/git/Cb/presentation/cb_interpreter_presentation.html"
    
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    content = update_async_samples(content)
    
    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(content)
    
    print("✓ Section 3のasync/awaitサンプルを3種類に分割しました")
