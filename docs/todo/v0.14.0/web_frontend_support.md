# Webフロントエンド開発サポート

**バージョン**: v0.16.0
**作成日**: 2025-11-13
**ステータス**: 追加設計

---

## 目次

1. [概要](#1-概要)
2. [HTML生成機能](#2-html生成機能)
3. [CSS生成機能](#3-css生成機能)
4. [コンポーネントシステム](#4-コンポーネントシステム)
5. [DOMバインディング](#5-domバインディング)
6. [状態管理](#6-状態管理)
7. [ビルドシステム](#7-ビルドシステム)

---

## 1. 概要

### 1.1 不足していた機能

現在のWASM/TypeScriptバックエンドでは、以下のWebフロントエンド開発機能が不足しています：

**不足している機能**:
- ✗ HTML生成・テンプレート
- ✗ CSS生成・スタイリング
- ✗ JSX/TSX的なコンポーネント構文
- ✗ DOMバインディング
- ✗ イベントハンドリング
- ✗ 状態管理

### 1.2 追加する機能

v0.16.0に以下のWebフロントエンド機能を追加します：

**追加機能**:
- ✓ HTML生成（テンプレート構文）
- ✓ CSS生成（型安全なスタイリング）
- ✓ コンポーネントベース開発
- ✓ リアクティブな状態管理
- ✓ イベントバインディング
- ✓ React/Vue風の開発体験

---

## 2. HTML生成機能

### 2.1 HTMLテンプレート構文

#### Cb言語でのHTML構文

```cb
// HTMLビルダーAPI
Html render_page() {
    return html {
        head {
            title { "My Cb App" }
            meta(charset="UTF-8")
            link(rel="stylesheet", href="style.css")
        }
        body {
            div(class="container") {
                h1(id="title") { "Hello, Cb!" }
                p { "This is a web app written in Cb language." }
                button(onclick=handle_click) { "Click me!" }
            }
        }
    };
}

// JSX/TSX風の構文（マクロで実装）
Html render_component() {
    return jsx! {
        <div class="card">
            <h2>{title}</h2>
            <p>{content}</p>
            <button onclick={on_click}>Submit</button>
        </div>
    };
}
```

#### HTMLビルダーAPI

```cb
// runtime/web/html.cb

// HTML要素
struct HtmlElement {
    tag: string;
    attributes: Map<string, string>;
    children: Vec<HtmlNode>;
}

enum HtmlNode {
    Element(HtmlElement),
    Text(string),
}

// HTMLビルダー
struct HtmlBuilder {
    HtmlBuilder new();

    // 要素の作成
    ElementBuilder element(self, string tag);

    // テキストノード
    HtmlNode text(self, string content);

    // レンダリング
    string render(self);
}

struct ElementBuilder {
    ElementBuilder attr(self, string name, string value);
    ElementBuilder child(self, HtmlNode node);
    ElementBuilder text(self, string content);
    HtmlNode build(self);
}

// ヘルパー関数
HtmlNode div(Map<string, string> attributes, Vec<HtmlNode> children) {
    return HtmlElement {
        tag: "div",
        attributes: attributes,
        children: children,
    };
}

HtmlNode  h1(attributes: Map<string, string>, children: Vec<HtmlNode>) {
    return HtmlElement {
        tag: "h1",
        attributes: attributes,
        children: children,
    };
}

// 他のHTML要素も同様に定義...
```

### 2.2 テンプレート展開

#### ASTレベルでの表現

```cpp
// src/common/ast.h に追加

enum ASTNodeType {
    // 既存...
    AST_HTML_TEMPLATE,        // HTMLテンプレート
    AST_HTML_ELEMENT,         // HTML要素
    AST_HTML_TEXT,            // テキストノード
};

struct ASTHtmlElement {
    std::string tag_name;
    std::unordered_map<std::string, std::unique_ptr<ASTNode>> attributes;
    std::vector<std::unique_ptr<ASTNode>> children;
};
```

#### TypeScript生成

```cb
// 関数ポインタ型を定義
typedef ClickHandler = void();

// Cbコード
Html render_button(string text, ClickHandler handler) {
    return html {
        button(onclick=handler, class="btn") { text }
    };
}

// 生成されるTypeScript
export function render_button(text: string, handler: () => void): HTMLElement {
    const button = document.createElement('button');
    button.className = 'btn';
    button.textContent = text;
    button.onclick = handler;
    return button;
}
```

---

## 3. CSS生成機能

### 3.1 型安全なCSS構文

#### Cb言語でのCSS定義

```cb
// スタイルの定義
StyleSheet  define_styles() {
    return css {
        ".container" {
            display: "flex";
            flex_direction: "column";
            padding: px(20);
            background_color: rgb(255, 255, 255);
        }

        ".card" {
            border: solid(px(1), rgb(200, 200, 200));
            border_radius: px(8);
            padding: px(16);
            margin: px(10);

            "&:hover" {  // ネストしたセレクタ
                box_shadow: shadow(0, 2, 4, rgba(0, 0, 0, 0.1));
            }
        }

        ".btn" {
            background_color: rgb(0, 122, 255);
            color: rgb(255, 255, 255);
            padding: px(10) px(20);
            border: none;
            border_radius: px(4);
            cursor: "pointer";

            "&:hover" {
                background_color: rgb(0, 100, 230);
            }
        }
    };
}
```

#### CSSビルダーAPI

```cb
// runtime/web/css.cb

// CSS値の型
enum CssValue {
    Px(int),
    Em(float),
    Rem(float),
    Percent(int),
    Color(Color),
    String(string),
}

struct Color {
    r: int;
    g: int;
    b: int;
    a: float;
}

// CSSルール
struct CssRule {
    selector: string;
    properties: Map<string, CssValue>;
    nested_rules: Vec<CssRule>;
}

// スタイルシート
struct StyleSheet {
    rules: Vec<CssRule>;

    string render(self);
}

// ヘルパー関数
CssValue  px(value: int) {
    return CssValue::Px(value);
}

CssValue  rgb(r: int, g: int, b: int) {
    return CssValue::Color(Color { r, g, b, a: 1.0 });
}

CssValue  rgba(r: int, g: int, b: int, a: float) {
    return CssValue::Color(Color { r, g, b, a });
}
```

### 3.2 CSS生成

#### 生成されるCSS

```css
/* Cbコードから生成 */
.container {
    display: flex;
    flex-direction: column;
    padding: 20px;
    background-color: rgb(255, 255, 255);
}

.card {
    border: 1px solid rgb(200, 200, 200);
    border-radius: 8px;
    padding: 16px;
    margin: 10px;
}

.card:hover {
    box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
}

.btn {
    background-color: rgb(0, 122, 255);
    color: rgb(255, 255, 255);
    padding: 10px 20px;
    border: none;
    border-radius: 4px;
    cursor: pointer;
}

.btn:hover {
    background-color: rgb(0, 100, 230);
}
```

### 3.3 CSSインライン化

```cb
// インラインスタイル
Html  render_styled_div() {
    let style = inline_style {
        background_color: rgb(240, 240, 240);
        padding: px(10);
        border_radius: px(5);
    };

    return div(style=style) {
        "Styled content"
    };
}

// 生成されるHTML
// <div style="background-color: rgb(240, 240, 240); padding: 10px; border-radius: 5px;">
//   Styled content
// </div>
```

---

## 4. コンポーネントシステム

### 4.1 コンポーネントの定義

#### Cb言語でのコンポーネント

```cb
// カウンターコンポーネント
struct Counter {
    count: int;

    Counter  new() {
        return Counter { count: 0 };
    }

    Html  render(self) {
        return div(class="counter") {
            h2 { "Counter: {self.count}" }
            button(onclick=self.increment) { "Increment" }
            button(onclick=self.decrement) { "Decrement" }
        };
    }

    fn increment(mut self) {
        self.count += 1;
        self.re_render();
    }

    fn decrement(mut self) {
        self.count -= 1;
        self.re_render();
    }
}

// Todoリストコンポーネント
struct TodoList {
    items: Vec<TodoItem>;

    Html  render(self) {
        return div(class="todo-list") {
            h2 { "Todo List" }
            ul {
                for item in self.items {
                    li(key=item.id) {
                        input(
                            type="checkbox",
                            checked=item.completed,
                            onchange=|_| self.toggle(item.id)
                        )
                        span(class=if item.completed { "completed" } else { "" }) {
                            item.text
                        }
                        button(onclick=|_| self.remove(item.id)) { "Delete" }
                    }
                }
            }
            input(
                type="text",
                placeholder="New todo...",
                onkeypress=self.handle_keypress
            )
        };
    }
}
```

### 4.2 コンポーネントライフサイクル

```cb
// コンポーネント trait
interface Component {
    Html render(self);
    fn on_mount(mut self) {}
    fn on_update(mut self) {}
    fn on_unmount(mut self) {}
}

// ライフサイクル付きコンポーネント
struct MyComponent {
    data: string;
}

impl Component for MyComponent {
    Html  render(self) {
        return div { self.data };
    }

    fn on_mount(mut self) {
        println("Component mounted!");
        // 初期化処理
    }

    fn on_update(mut self) {
        println("Component updated!");
        // 更新処理
    }

    fn on_unmount(mut self) {
        println("Component unmounted!");
        // クリーンアップ処理
    }
}
```

---

## 5. DOMバインディング

### 5.1 リアクティブバインディング

```cb
// リアクティブな状態
struct AppState {
    @reactive
    name: string;

    @reactive
    count: int;
}

Html  render_app(state: AppState) {
    return div {
        // 双方向バインディング
        input(
            type="text",
            value=state.name,
            oninput=|e| state.name = e.target.value
        )

        // 一方向バインディング
        p { "Hello, {state.name}!" }

        // 条件付きレンダリング
        if state.count > 10 {
            div(class="warning") { "Count is high!" }
        }

        // リストレンダリング
        ul {
            for i in 0..state.count {
                li(key=i) { "Item {i}" }
            }
        }
    };
}
```

### 5.2 イベントハンドリング

```cb
// イベントハンドラ
// 関数ポインタ型を定義
typedef ClickHandler = void(MouseEvent);

struct Button {
    string label;
    ClickHandler on_click;

    Html render(self) {
        return button(onclick=self.on_click) { self.label };
    }
}

// 使用例
Button  create_submit_button() {
    return Button {
        label: "Submit",
        on_click: |event| {
            event.prevent_default();
            println("Button clicked!");
            // フォーム送信処理
        }
    };
}

// イベントの種類
enum Event {
    Mouse(MouseEvent),
    Keyboard(KeyboardEvent),
    Input(InputEvent),
    Form(FormEvent),
}

struct MouseEvent {
    x: int;
    y: int;
    button: int;
    fn prevent_default(self);
    fn stop_propagation(self);
}

struct KeyboardEvent {
    key: string;
    code: string;
    shift_key: bool;
    ctrl_key: bool;
    alt_key: bool;
}
```

---

## 6. 状態管理

### 6.1 シンプルな状態管理

```cb
// 関数ポインタ型を定義
typedef StateListener<T> = void(T);

// Store パターン
struct Store<T> {
    T state;
    Vec<StateListener<T>> subscribers;

    Store<T> new(T initial_state) {
        return Store {
            state: initial_state,
            subscribers: Vec::new()
        };
    }

    T get_state(self) {
        return self.state;
    }

    void set_state(mut self, T new_state) {
        self.state = new_state;
        self.notify();
    }

    void subscribe(mut self, StateListener<T> callback) {
        self.subscribers.push(callback);
    }

    fn notify(self) {
        for subscriber in self.subscribers {
            subscriber(self.state);
        }
    }
}

// 使用例
struct AppState {
    user: User;
    todos: Vec<Todo>;
}

let store = Store::new(AppState {
    user: User::guest(),
    todos: Vec::new()
});

// 状態の購読
store.subscribe(|state| {
    re_render(state);
});

// 状態の更新
store.set_state(AppState {
    user: current_user,
    todos: updated_todos
});
```

### 6.2 Redux風の状態管理

```cb
// Action
enum Action {
    AddTodo(string),
    ToggleTodo(int),
    RemoveTodo(int),
    SetFilter(TodoFilter),
}

// Reducer
TodoState  todo_reducer(state: TodoState, action: Action) {
    match action {
        Action::AddTodo(text) => {
            let new_todo = Todo {
                id: state.next_id,
                text: text,
                completed: false
            };
            return TodoState {
                todos: state.todos.push(new_todo),
                next_id: state.next_id + 1,
                ..state
            };
        }
        Action::ToggleTodo(id) => {
            let updated_todos = state.todos.map(|todo| {
                if todo.id == id {
                    return Todo { completed: !todo.completed, ..todo };
                }
                return todo;
            });
            return TodoState { todos: updated_todos, ..state };
        }
        // 他のアクション...
    }
}

// 関数ポインタ型を定義
typedef Reducer<S, A> = S(S, A);
typedef StateSubscriber<S> = void(S);

// Store
struct Redux<S, A> {
    S state;
    Reducer<S, A> reducer;
    Vec<StateSubscriber<S>> subscribers;

    void dispatch(mut self, A action) {
        self.state = (self.reducer)(self.state, action);
        self.notify();
    }
}
```

---

## 7. ビルドシステム

### 7.1 Webアプリのビルド

#### コマンドライン

```bash
# Webアプリ用にビルド
./main app.cb \
    --backend=wasm \
    --target=wasm32 \
    --output=dist/app.wasm \
    --emit-html \
    --emit-css

# TypeScriptとして出力
./main app.cb \
    --backend=typescript \
    --output=dist/app.ts \
    --emit-html \
    --emit-css

# 開発サーバー起動
./main app.cb --serve --watch
```

#### プロジェクト構造

```
my-web-app/
├── src/
│   ├── main.cb           # エントリーポイント
│   ├── components/
│   │   ├── Header.cb
│   │   ├── Footer.cb
│   │   └── TodoList.cb
│   ├── styles/
│   │   └── app.cb        # CSS定義
│   └── state/
│       └── store.cb
├── public/
│   └── index.html        # ベースHTML
├── dist/                 # ビルド出力
│   ├── app.wasm
│   ├── app.js
│   ├── app.css
│   └── index.html
└── cb.toml               # プロジェクト設定
```

### 7.2 cb.toml設定

```toml
[package]
name = "my-web-app"
version = "0.1.0"
authors = ["Your Name <you@example.com>"]

[build]
backend = "wasm"
target = "wasm32"
output-dir = "dist"

[web]
emit-html = true
emit-css = true
entry-point = "src/main.cb"

# HTMLテンプレート
[web.template]
title = "My Cb Web App"
meta-charset = "UTF-8"
meta-viewport = "width=device-width, initial-scale=1.0"

# CSS設定
[web.css]
minify = true
autoprefixer = true

# 開発サーバー
[dev-server]
port = 3000
hot-reload = true
```

### 7.3 ビルドスクリプト

```bash
#!/bin/bash
# build-web.sh

# Cbコンパイラでビルド
./main src/main.cb \
    --backend=wasm \
    --target=wasm32 \
    --output=dist/app.wasm \
    --emit-html \
    --emit-css

# WASMバインディング生成
wasm-bindgen dist/app.wasm \
    --out-dir dist \
    --target web

# HTMLファイル生成
cat > dist/index.html <<EOF
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>My Cb Web App</title>
    <link rel="stylesheet" href="app.css">
</head>
<body>
    <div id="app"></div>
    <script type="module" src="app.js"></script>
</body>
</html>
EOF

# 開発サーバー起動（オプション）
if [ "$1" = "--serve" ]; then
    python3 -m http.server 3000 --directory dist
fi
```

---

## 8. 完全な例

### 8.1 Todoアプリケーション

```cb
// src/main.cb

import web::{html, css, Component, Store};

// 状態の定義
struct Todo {
    id: int;
    text: string;
    completed: bool;
}

struct AppState {
    todos: Vec<Todo>;
    filter: TodoFilter;
    next_id: int;
}

enum TodoFilter {
    All,
    Active,
    Completed,
}

// メインコンポーネント
struct TodoApp {
    store: Store<AppState>;

    TodoApp  new() {
        let initial_state = AppState {
            todos: Vec::new(),
            filter: TodoFilter::All,
            next_id: 1,
        };

        return TodoApp {
            store: Store::new(initial_state)
        };
    }

    Html  render(self) {
        let state = self.store.get_state();

        return div(class="app") {
            h1 { "Cb Todo App" }

            // 入力フォーム
            self.render_input()

            // Todoリスト
            self.render_todos(state)

            // フィルター
            self.render_filters(state)

            // 統計
            self.render_stats(state)
        };
    }

    Html  render_input(self) {
        return div(class="input-container") {
            input(
                type="text",
                placeholder="What needs to be done?",
                onkeypress=|e| {
                    if e.key == "Enter" {
                        self.add_todo(e.target.value);
                        e.target.value = "";
                    }
                }
            )
        };
    }

    Html  render_todos(self, state: AppState) {
        let filtered_todos = self.filter_todos(state.todos, state.filter);

        return ul(class="todo-list") {
            for todo in filtered_todos {
                li(key=todo.id, class=if todo.completed { "completed" } else { "" }) {
                    input(
                        type="checkbox",
                        checked=todo.completed,
                        onchange=|_| self.toggle_todo(todo.id)
                    )
                    span { todo.text }
                    button(
                        class="delete",
                        onclick=|_| self.remove_todo(todo.id)
                    ) { "×" }
                }
            }
        };
    }

    fn add_todo(mut self, text: string) {
        let mut state = self.store.get_state();
        state.todos.push(Todo {
            id: state.next_id,
            text: text,
            completed: false
        });
        state.next_id += 1;
        self.store.set_state(state);
    }

    fn toggle_todo(mut self, id: int) {
        let mut state = self.store.get_state();
        for i in 0..state.todos.len() {
            if state.todos[i].id == id {
                state.todos[i].completed = !state.todos[i].completed;
                break;
            }
        }
        self.store.set_state(state);
    }

    fn remove_todo(mut self, id: int) {
        let mut state = self.store.get_state();
        state.todos = state.todos.filter(|todo| todo.id != id);
        self.store.set_state(state);
    }
}

// スタイルの定義
StyleSheet  define_styles() {
    return css {
        ".app" {
            max_width: px(600);
            margin: "0 auto";
            padding: px(20);
            font_family: "Arial, sans-serif";
        }

        ".input-container input" {
            width: percent(100);
            padding: px(10);
            font_size: px(16);
            border: solid(px(2), rgb(220, 220, 220));
            border_radius: px(4);
        }

        ".todo-list" {
            list_style: "none";
            padding: 0;
        }

        ".todo-list li" {
            display: "flex";
            align_items: "center";
            padding: px(10);
            border_bottom: solid(px(1), rgb(240, 240, 240));
        }

        ".todo-list li.completed span" {
            text_decoration: "line-through";
            color: rgb(150, 150, 150);
        }

        ".delete" {
            margin_left: "auto";
            background: "none";
            border: "none";
            color: rgb(200, 0, 0);
            font_size: px(20);
            cursor: "pointer";
        }
    };
}

// エントリーポイント
fn main() {
    let app = TodoApp::new();

    // スタイルの適用
    let styles = define_styles();
    apply_styles(styles);

    // アプリのマウント
    mount("#app", app);
}
```

---

## 9. まとめ

これらの機能追加により、v0.16.0は完全なWebフロントエンド開発をサポートします：

**HTML生成**:
- ✓ テンプレート構文
- ✓ JSX/TSX風の記法
- ✓ 型安全なDOM構築

**CSS生成**:
- ✓ 型安全なスタイル定義
- ✓ ネストしたセレクタ
- ✓ インラインスタイル

**コンポーネント**:
- ✓ React風のコンポーネントシステム
- ✓ ライフサイクルメソッド
- ✓ 再利用可能なUI部品

**状態管理**:
- ✓ リアクティブな状態
- ✓ Redux風のパターン
- ✓ 双方向バインディング

これで、Cb言語を使って本格的なWebアプリケーション開発が可能になります！
