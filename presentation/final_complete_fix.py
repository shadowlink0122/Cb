#!/usr/bin/env python3
"""
Final comprehensive fix for Cb presentation
All modifications requested by user
"""
import re

def add_interpreter_implementation_slides(content):
    """Add slides about interpreter implementation (interface/impl and async/await)"""
    
    # Find the location before "セクション5"
    section5_marker = '<!-- ========================================'
    section5_text = 'セクション5: まとめと今後（3分）'
    
    new_slides = '''
            <!-- ========================================
                 セクション4: インタプリタの内部実装（4分）
                 ======================================== -->

            <!-- セクション区切り -->
            <section class="section-divider">
                <h1>Section 4</h1>
                <h2>インタプリタの内部実装</h2>
                <p class="small-text">Interface/Impl と 協調的マルチタスクの実装方法</p>
            </section>

            <!-- スライド: Interface/Implの実装方法 -->
            <section class="left-align">
                <h2>Interface/Implの実装方法</h2>
                <div style="display: flex; gap: 1.0em; width: 95%; margin: 0.3em auto 0;">
                    <div style="flex: 1;">
                        <h3 style="font-size: 0.75em; margin-bottom: 0.2em;">実装の概要</h3>
                        <ul style="font-size: 0.63em; line-height: 1.3; margin: 0 0 0.5em 0; padding-left: 1.3em;">
                            <li><strong>Interface</strong>: 抽象メソッド定義を保持</li>
                            <li><strong>Impl</strong>: 具体的な実装を登録</li>
                            <li><strong>動的ディスパッチ</strong>: 実行時に適切なメソッドを呼び出し</li>
                            <li><strong>型安全性</strong>: コンパイル時に型チェック</li>
                        </ul>
                        <h3 style="font-size: 0.75em; margin-bottom: 0.2em;">データ構造（C++）</h3>
                        <pre style="font-size: 0.42em; margin: 0.3em 0;"><code class="language-cpp" data-trim>
<span style="color: #6a9955;">// Interface定義を保持</span>
<span style="color: #c586c0; font-weight: bold;">struct</span> <span style="color: #4ec9b0;">InterfaceDefinition</span> {
    std::<span style="color: #4ec9b0;">string</span> name;
    std::<span style="color: #4ec9b0;">vector</span>&lt;<span style="color: #4ec9b0;">Method</span>&gt; methods;
};

<span style="color: #6a9955;">// Impl実装を登録</span>
<span style="color: #c586c0; font-weight: bold;">struct</span> <span style="color: #4ec9b0;">ImplRegistration</span> {
    std::<span style="color: #4ec9b0;">string</span> interface_name;
    std::<span style="color: #4ec9b0;">string</span> type_name;
    std::<span style="color: #4ec9b0;">map</span>&lt;std::<span style="color: #4ec9b0;">string</span>, <span style="color: #4ec9b0;">ASTNode</span>*&gt; 
        method_impls;
};
                        </code></pre>
                    </div>
                    <div style="flex: 1;">
                        <h3 style="font-size: 0.75em; margin-bottom: 0.2em;">実行時の動的ディスパッチ</h3>
                        <pre style="font-size: 0.42em; margin: 0.3em 0;"><code class="language-cpp" data-trim>
<span style="color: #6a9955;">// メソッド呼び出し時の処理</span>
<span style="color: #4ec9b0;">TypedValue</span> call_interface_method(
    <span style="color: #4ec9b0;">TypedValue</span>& obj,
    std::<span style="color: #4ec9b0;">string</span> method_name,
    std::<span style="color: #4ec9b0;">vector</span>&lt;<span style="color: #4ec9b0;">TypedValue</span>&gt;& args) {
    
    <span style="color: #6a9955;">// 1. オブジェクトの型を取得</span>
    std::<span style="color: #4ec9b0;">string</span> type_name = 
        obj.<span style="color: #dcdcaa;">get_type_name</span>();
    
    <span style="color: #6a9955;">// 2. Interface→Impl のマッピングを検索</span>
    <span style="color: #4ec9b0;">ImplRegistration</span>* impl = 
        find_impl(interface_name, type_name);
    
    <span style="color: #6a9955;">// 3. 該当メソッドの実装ASTを取得</span>
    <span style="color: #4ec9b0;">ASTNode</span>* method_ast = 
        impl->method_impls[method_name];
    
    <span style="color: #6a9955;">// 4. スコープを作成してメソッドを実行</span>
    <span style="color: #4ec9b0;">Scope</span> method_scope;
    method_scope.<span style="color: #dcdcaa;">set_variable</span>(
        <span style="color: #ce9178;">"self"</span>, obj);  <span style="color: #6a9955;">// selfを設定</span>
    
    <span style="color: #c586c0; font-weight: bold;">return</span> evaluate(method_ast, method_scope);
}
                        </code></pre>
                        <p style="font-size: 0.60em; margin-top: 0.3em;">
                            <strong class="highlight">ポイント</strong>: Goのような暗黙的実装ではなく、<code>impl</code>で明示的に登録
                        </p>
                    </div>
                </div>
            </section>

            <!-- スライド: 協調的マルチタスクの実装 -->
            <section class="left-align">
                <h2>協調的マルチタスク（async/await）の実装</h2>
                <div style="width: 95%; margin: 0.3em auto 0;">
                    <div style="display: flex; gap: 1.0em; margin-bottom: 0.5em;">
                        <div style="flex: 1;">
                            <h3 style="font-size: 0.75em; margin-bottom: 0.2em;">アーキテクチャ概要</h3>
                            <ul style="font-size: 0.63em; line-height: 1.3; margin: 0; padding-left: 1.3em;">
                                <li><strong>Event Loop</strong>: タスクキュー管理</li>
                                <li><strong>Future&lt;T&gt;</strong>: 非同期タスクの状態を表現</li>
                                <li><strong>await</strong>: 制御をEvent Loopに戻す</li>
                                <li><strong>協調的スケジューリング</strong>: タスク自身が制御を譲渡</li>
                            </ul>
                        </div>
                        <div style="flex: 1;">
                            <h3 style="font-size: 0.75em; margin-bottom: 0.2em;">Future&lt;T&gt;の状態管理</h3>
                            <pre style="font-size: 0.40em; margin: 0.3em 0;"><code class="language-cpp" data-trim>
<span style="color: #c586c0; font-weight: bold;">enum</span> <span style="color: #4ec9b0;">FutureState</span> {
    PENDING,    <span style="color: #6a9955;">// 実行中</span>
    READY,      <span style="color: #6a9955;">// 完了</span>
    WAITING     <span style="color: #6a9955;">// 待機中（sleep等）</span>
};

<span style="color: #c586c0; font-weight: bold;">struct</span> <span style="color: #4ec9b0;">Future</span>&lt;<span style="color: #4ec9b0;">T</span>&gt; {
    <span style="color: #4ec9b0;">FutureState</span> state;
    <span style="color: #4ec9b0;">T</span> value;          <span style="color: #6a9955;">// 結果値</span>
    <span style="color: #4ec9b0;">long</span> wake_time;   <span style="color: #6a9955;">// 再開時刻</span>
    <span style="color: #4ec9b0;">ASTNode</span>* continuation;
};
                            </code></pre>
                        </div>
                    </div>
                    
                    <h3 style="font-size: 0.75em; margin-bottom: 0.2em;">Event Loopの実装</h3>
                    <pre style="font-size: 0.45em; margin: 0.3em 0;"><code class="language-cpp" data-trim>
<span style="color: #c586c0; font-weight: bold;">class</span> <span style="color: #4ec9b0;">EventLoop</span> {
    std::<span style="color: #4ec9b0;">deque</span>&lt;<span style="color: #4ec9b0;">Task</span>*&gt; task_queue;
    std::<span style="color: #4ec9b0;">map</span>&lt;<span style="color: #4ec9b0;">int</span>, <span style="color: #4ec9b0;">Future</span>*&gt; pending_futures;
    <span style="color: #4ec9b0;">int</span> next_task_id = <span style="color: #b5cea8;">0</span>;

<span style="color: #c586c0; font-weight: bold;">public</span>:
    <span style="color: #6a9955;">// async関数を実行</span>
    <span style="color: #4ec9b0;">int</span> spawn_task(<span style="color: #4ec9b0;">ASTNode</span>* async_func, <span style="color: #4ec9b0;">Scope</span>& scope) {
        <span style="color: #4ec9b0;">Task</span>* task = <span style="color: #c586c0; font-weight: bold;">new</span> <span style="color: #4ec9b0;">Task</span>{next_task_id++, async_func, scope};
        task_queue.<span style="color: #dcdcaa;">push_back</span>(task);
        <span style="color: #c586c0; font-weight: bold;">return</span> task->id;
    }
    
    <span style="color: #6a9955;">// Event Loopを実行</span>
    <span style="color: #4ec9b0;">void</span> run() {
        <span style="color: #c586c0; font-weight: bold;">while</span> (!task_queue.<span style="color: #dcdcaa;">empty</span>()) {
            <span style="color: #4ec9b0;">Task</span>* task = task_queue.<span style="color: #dcdcaa;">front</span>();
            task_queue.<span style="color: #dcdcaa;">pop_front</span>();
            
            <span style="color: #6a9955;">// await式を評価（制御を戻す可能性あり）</span>
            <span style="color: #4ec9b0;">TypedValue</span> result = evaluate_task(task);
            
            <span style="color: #c586c0; font-weight: bold;">if</span> (result.<span style="color: #dcdcaa;">is_pending</span>()) {
                <span style="color: #6a9955;">// まだ完了していない → キューに戻す</span>
                <span style="color: #c586c0; font-weight: bold;">if</span> (current_time_ms() >= task->wake_time) {
                    task_queue.<span style="color: #dcdcaa;">push_back</span>(task);  <span style="color: #6a9955;">// すぐ再実行</span>
                } <span style="color: #c586c0; font-weight: bold;">else</span> {
                    task_queue.<span style="color: #dcdcaa;">push_back</span>(task);  <span style="color: #6a9955;">// 後で再チェック</span>
                }
            } <span style="color: #c586c0; font-weight: bold;">else</span> {
                <span style="color: #6a9955;">// 完了 → Futureを解決</span>
                resolve_future(task->id, result);
                <span style="color: #c586c0; font-weight: bold;">delete</span> task;
            }
        }
    }
    
    <span style="color: #6a9955;">// await式の評価</span>
    <span style="color: #4ec9b0;">TypedValue</span> evaluate_await(<span style="color: #4ec9b0;">ASTNode</span>* await_expr, <span style="color: #4ec9b0;">Scope</span>& scope) {
        <span style="color: #6a9955;">// 1. awaitする式を評価</span>
        <span style="color: #4ec9b0;">TypedValue</span> future_value = evaluate(await_expr->child, scope);
        
        <span style="color: #6a9955;">// 2. Futureがまだ完了していない場合</span>
        <span style="color: #c586c0; font-weight: bold;">if</span> (future_value.<span style="color: #dcdcaa;">is_pending</span>()) {
            <span style="color: #c586c0; font-weight: bold;">return</span> <span style="color: #4ec9b0;">TypedValue</span>::<span style="color: #4ec9b0;">Pending</span>();  <span style="color: #6a9955;">// Event Loopに制御を戻す</span>
        }
        
        <span style="color: #6a9955;">// 3. Futureが完了している → 値を取り出す</span>
        <span style="color: #c586c0; font-weight: bold;">return</span> future_value.<span style="color: #dcdcaa;">unwrap</span>();  <span style="color: #6a9955;">// Result/Optionのenum情報を保持</span>
    }
};
                    </code></pre>
                    <p class="fragment" style="font-size: 0.65em; margin-top: 0.3em;">
                        <strong class="success">キーポイント</strong>: タスクが自発的に制御を譲渡（yield）するため、デッドロックやレースコンディションが発生しない
                    </p>
                </div>
            </section>

            <!-- スライド: Result型とasync/awaitの統合実装 -->
            <section class="left-align">
                <h2>v0.13.0の実装: Result型とasync/awaitの統合</h2>
                <div style="width: 95%; margin: 0.3em auto 0;">
                    <h3 style="font-size: 0.75em; margin-bottom: 0.2em;">技術的課題と解決策</h3>
                    <div style="display: flex; gap: 1.0em; margin-bottom: 0.5em;">
                        <div style="flex: 1;">
                            <h4 style="font-size: 0.70em; margin-bottom: 0.2em;">課題</h4>
                            <ul style="font-size: 0.60em; line-height: 1.3; margin: 0; padding-left: 1.3em;">
                                <li>async関数がResult&lt;T,E&gt;を返す際、Futureでラップするとenum情報（Ok/Err）が失われる</li>
                                <li>awaitした後にmatchできない</li>
                                <li>型パラメータの保持が困難</li>
                            </ul>
                        </div>
                        <div style="flex: 1;">
                            <h4 style="font-size: 0.70em; margin-bottom: 0.2em;">解決策</h4>
                            <ul style="font-size: 0.60em; line-height: 1.3; margin: 0; padding-left: 1.3em;">
                                <li><strong>TypedValue拡張</strong>: variant名と型パラメータを保持</li>
                                <li><strong>evaluate_await()改善</strong>: enum情報を完全に保持して返却</li>
                                <li><strong>暗黙的Future化</strong>: async関数は自動的にFutureになるが、await時にアンラップ</li>
                            </ul>
                        </div>
                    </div>
                    <h3 style="font-size: 0.75em; margin-bottom: 0.2em;">実装コード（C++）</h3>
                    <pre style="font-size: 0.42em; margin: 0.3em 0;"><code class="language-cpp" data-trim>
<span style="color: #6a9955;">// TypedValueにenum情報を保持</span>
<span style="color: #c586c0; font-weight: bold;">struct</span> <span style="color: #4ec9b0;">TypedValue</span> {
    <span style="color: #4ec9b0;">Type</span> type;
    std::<span style="color: #4ec9b0;">any</span> value;
    
    <span style="color: #6a9955;">// v0.13.0で追加: enum型の情報</span>
    <span style="color: #4ec9b0;">bool</span> is_enum = <span style="color: #c586c0; font-weight: bold;">false</span>;
    std::<span style="color: #4ec9b0;">string</span> enum_variant;        <span style="color: #6a9955;">// "Ok", "Err", "Some", "None"</span>
    std::<span style="color: #4ec9b0;">vector</span>&lt;<span style="color: #4ec9b0;">Type</span>&gt; type_params;    <span style="color: #6a9955;">// &lt;int, string&gt; など</span>
};

<span style="color: #6a9955;">// evaluate_await()の実装</span>
<span style="color: #4ec9b0;">TypedValue</span> Interpreter::evaluate_await(<span style="color: #4ec9b0;">ASTNode</span>* await_node, <span style="color: #4ec9b0;">Scope</span>& scope) {
    <span style="color: #6a9955;">// 1. async関数を評価（暗黙的にFuture&lt;Result&lt;T,E&gt;&gt;になる）</span>
    <span style="color: #4ec9b0;">TypedValue</span> future_val = evaluate(await_node->awaited_expr, scope);
    
    <span style="color: #6a9955;">// 2. Futureが完了するまで待機</span>
    <span style="color: #c586c0; font-weight: bold;">while</span> (future_val.<span style="color: #dcdcaa;">state</span>() == <span style="color: #4ec9b0;">FutureState</span>::PENDING) {
        event_loop.<span style="color: #dcdcaa;">process_tasks</span>();  <span style="color: #6a9955;">// 他のタスクを実行</span>
        future_val = check_future(future_val.<span style="color: #dcdcaa;">id</span>());
    }
    
    <span style="color: #6a9955;">// 3. ✅ Futureから値を取り出す際、enum情報を保持</span>
    <span style="color: #4ec9b0;">TypedValue</span> result = future_val.<span style="color: #dcdcaa;">get_value</span>();
    
    <span style="color: #6a9955;">// ✅ Result&lt;T,E&gt;のenum情報（Ok/Err）を保持したまま返却</span>
    result.is_enum = <span style="color: #c586c0; font-weight: bold;">true</span>;
    result.enum_variant = future_val.<span style="color: #dcdcaa;">inner_variant</span>();  <span style="color: #6a9955;">// "Ok" or "Err"</span>
    result.type_params = future_val.<span style="color: #dcdcaa;">inner_type_params</span>();  <span style="color: #6a9955;">// &lt;int, string&gt;</span>
    
    <span style="color: #c586c0; font-weight: bold;">return</span> result;  <span style="color: #6a9955;">// matchで正しく分岐可能！</span>
}
                    </code></pre>
                    <p class="fragment" style="font-size: 0.65em; margin-top: 0.3em;">
                        <strong class="success">成果</strong>: async関数がResult&lt;T,E&gt;を返しても、await後にmatchでOk/Errを正しく判定できる
                    </p>
                </div>
            </section>

            <!-- ========================================
'''
    
    # Find the section 5 marker and insert new slides before it
    pattern = r'(<!-- ========================================\s+セクション5: まとめと今後（3分）\s+======================================== -->)'
    
    content = re.sub(pattern, new_slides + r'\1', content, count=1)
    
    return content

def main():
    html_file = '/Users/shadowlink/Documents/git/Cb/presentation/cb_interpreter_presentation.html'
    
    print("Reading presentation...")
    with open(html_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    print("Adding interpreter implementation slides...")
    content = add_interpreter_implementation_slides(content)
    
    print("Writing back...")
    with open(html_file, 'w', encoding='utf-8') as f:
        f.write(content)
    
    print(f"✅ Successfully updated: {html_file}")
    print("Added:")
    print("  - Section 4: インタプリタの内部実装")
    print("  - Interface/Implの実装方法")
    print("  - 協調的マルチタスクの実装")
    print("  - Result型とasync/awaitの統合実装")

if __name__ == '__main__':
    main()
