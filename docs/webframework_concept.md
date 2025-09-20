# Cb言語 - Webフレームワーク設計構想

## 概要
Cb言語で実装される**フルスタックWebフレームワーク**の包括的設計文書。Rails/Reactの開発体験にRust/Goのパフォーマンスを組み合わせ、**単一言語でのフロントエンド・バックエンド統合開発**を実現します。

## 🎯 フレームワークのビジョン

### 目標
1. **統一開発体験**: フロント・バック・DBを同一言語で開発
2. **型安全性**: コンパイル時の包括的型チェック  
3. **高パフォーマンス**: Native + WebAssembly の最適化
4. **段階的導入**: 既存プロジェクトへの漸進的統合

### 技術スタック比較

| 技術スタック | 言語数 | 型安全性 | 性能 | 学習コスト |
|-------------|--------|----------|------|------------|
| **LAMP** | 4+ (HTML/CSS/JS/PHP) | 低 | 中 | 中 |
| **MEAN** | 3 (HTML/CSS/JS) | 中 | 中 | 中 |
| **Rails** | 4+ (HTML/CSS/JS/Ruby) | 中 | 中 | 中 |
| **Next.js** | 3 (HTML/CSS/TS) | 高 | 中 | 高 |
| **Cb Stack** | **2 (HTML/Cb)** | **最高** | **最高** | **低** |

## 🏗️ アーキテクチャ設計

### フルスタック統合アーキテクチャ

```
Cb Fullstack Application
├── 🖥️  Server (Native Binary)
│   ├── HTTP Server (async I/O)
│   ├── API Routing (/api/*)
│   ├── Business Logic
│   ├── Database ORM
│   └── Static File Serving
├── 🌐 Client (WebAssembly)
│   ├── Component System
│   ├── State Management  
│   ├── Router (SPA)
│   └── API Client
└── 🔄 Shared Code
    ├── Data Models (struct/enum)
    ├── API Contracts (interface)
    ├── Validation Logic
    └── Business Rules
```

### 開発・デプロイフロー

```
Development Mode:
├── cb dev --watch
│   ├── Server: Native Binary (Hot Reload)
│   ├── Client: WASM + JS Runtime (Live Reload)
│   └── Shared: Type Checking (Real-time)

Production Build:
├── cb build --release --target=web
│   ├── server.exe (高性能ネイティブバイナリ)
│   ├── client.wasm (最適化WebAssembly)  
│   ├── client.js (JavaScript fallback)
│   └── static/ (CSS/Assets)

Deployment:
├── 🐳 Docker Container
├── ☁️  Cloud Platform (AWS/GCP/Azure)
├── 🔧 CDN Distribution (静的ファイル)
└── 📊 Monitoring & Analytics
```

## 💻 実装例・コード設計

### バックエンド実装 (CbRails)
```cb
// === Shared Types (shared/models.cb) ===
#[derive(Serialize, Deserialize, Clone)]
struct User {
    id: UserId,
    email: Email,
    name: string,
    created_at: DateTime
}

#[derive(Serialize, Deserialize)]
struct CreateUserRequest {
    email: Email,
    name: string,
    password: Password
}

// API Contract Definition
interface UserService {
    async fn get_users(limit: Option<u32>) -> Result<Vec<User>, ApiError>;
    async fn create_user(req: CreateUserRequest) -> Result<User, ApiError>;
    async fn update_user(id: UserId, updates: UpdateUserRequest) -> Result<User, ApiError>;
    async fn delete_user(id: UserId) -> Result<(), ApiError>;
}

// === Server Implementation (server/main.cb) ===
use cb_web::{Server, Router, Database, Middleware};
use shared::models::*;

// Database Model with ORM
#[table("users")]
struct UserModel {
    #[primary_key]
    id: UserId,
    #[unique]
    email: Email,
    name: string,
    password_hash: string,
    created_at: DateTime,
    updated_at: DateTime
}

impl UserModel {
    async fn create(req: CreateUserRequest) -> Result<Self, DbError> {
        let hash = bcrypt::hash(&req.password)?;
        
        Self::insert()
            .email(req.email)
            .name(req.name)
            .password_hash(hash)
            .execute()
            .await
    }
    
    async fn find_by_email(email: &Email) -> Option<Self> {
        Self::select()
            .where_eq("email", email)
            .first()
            .await
            .ok()
    }
}

// Controller Implementation
struct UserController;

impl UserController {
    async fn index(req: Request) -> Result<Response, ApiError> {
        let limit = req.query("limit")?.parse().unwrap_or(20);
        let users = UserModel::select()
            .limit(limit)
            .execute()
            .await?;
            
        Ok(Response::json(users))
    }
    
    async fn create(req: Request) -> Result<Response, ApiError> {
        let create_req: CreateUserRequest = req.json().await?;
        
        // Validation
        validate_email(&create_req.email)?;
        validate_password(&create_req.password)?;
        
        // Check duplicates
        if UserModel::find_by_email(&create_req.email).await.is_some() {
            return Err(ApiError::Conflict("Email already exists"));
        }
        
        let user = UserModel::create(create_req).await?;
        Ok(Response::json(user).status(201))
    }
}

// Server Setup
#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let database = Database::connect("postgresql://localhost/myapp").await?;
    let mut router = Router::new();
    
    // Middleware
    router.use_middleware(cors_middleware());
    router.use_middleware(auth_middleware());
    router.use_middleware(logging_middleware());
    
    // API Routes
    router.scope("/api/v1", |api| {
        api.resource("/users", UserController);
        api.post("/auth/login", AuthController::login);
        api.post("/auth/logout", AuthController::logout);
    });
    
    // Static file serving
    router.static_files("/static", "./public");
    
    // SPA fallback
    router.get("/*", serve_spa_index);
    
    let server = Server::new()
        .bind("0.0.0.0:8080")
        .router(router)
        .database(database);
    
    println!("🚀 Cb server running on http://localhost:8080");
    server.run().await
}
```

### フロントエンド実装 (CbReact)
```cb
// === Client Implementation (client/app.cb) ===
use cb_web_client::{Component, Html, html, ApiClient, Router};
use shared::models::*;

// API Client (自動生成)
struct ApiClient {
    base_url: string,
    auth_token: Option<string>
}

impl UserService for ApiClient {
    async fn get_users(&self, limit: Option<u32>) -> Result<Vec<User>, ApiError> {
        let url = format!("/api/v1/users?limit={}", limit.unwrap_or(20));
        self.get(url).await
    }
    
    async fn create_user(&self, req: CreateUserRequest) -> Result<User, ApiError> {
        self.post("/api/v1/users", req).await
    }
}

// Component Definition
#[component]
struct UserList {
    users: Vec<User>,
    loading: bool,
    error: Option<string>,
    api: ApiClient
}

impl Component for UserList {
    type Message = UserListMsg;
    
    fn create() -> Self {
        Self {
            users: Vec::new(),
            loading: true,
            error: None,
            api: ApiClient::new("/api")
        }
    }
    
    fn mounted(&mut self) {
        self.load_users();
    }
    
    fn update(&mut self, msg: Self::Message) {
        match msg {
            UserListMsg::LoadUsers => self.load_users(),
            UserListMsg::UsersLoaded(users) => {
                self.users = users;
                self.loading = false;
            },
            UserListMsg::LoadError(error) => {
                self.error = Some(error);
                self.loading = false;
            },
            UserListMsg::DeleteUser(id) => self.delete_user(id),
        }
    }
    
    fn render(&self) -> Html {
        html! {
            <div class="user-list">
                <h2>"Users"</h2>
                
                {if self.loading {
                    <div class="loading">"Loading users..."</div>
                } else if let Some(error) = &self.error {
                    <div class="error">{"Error: " + error}</div>
                } else {
                    <div>
                        <button onclick={|_| self.send_message(UserListMsg::LoadUsers)}>
                            "Reload"
                        </button>
                        
                        <div class="users">
                            {for user in &self.users {
                                <UserCard 
                                    user={user.clone()} 
                                    on_delete={move |id| self.send_message(UserListMsg::DeleteUser(id))}
                                />
                            }}
                        </div>
                        
                        {if self.users.is_empty() {
                            <p>"No users found"</p>
                        }}
                    </div>
                }}
            </div>
        }
    }
}

impl UserList {
    async fn load_users(&mut self) {
        self.loading = true;
        self.error = None;
        
        match self.api.get_users(Some(50)).await {
            Ok(users) => self.send_message(UserListMsg::UsersLoaded(users)),
            Err(e) => self.send_message(UserListMsg::LoadError(e.to_string()))
        }
    }
    
    async fn delete_user(&mut self, id: UserId) {
        if let Err(e) = self.api.delete_user(id).await {
            self.send_message(UserListMsg::LoadError(e.to_string()));
        } else {
            self.load_users();
        }
    }
}

// Message Types
enum UserListMsg {
    LoadUsers,
    UsersLoaded(Vec<User>),
    LoadError(string),
    DeleteUser(UserId)
}

// User Card Component
#[component]
struct UserCard {
    user: User,
    on_delete: fn(UserId)
}

impl Component for UserCard {
    fn render(&self) -> Html {
        html! {
            <div class="user-card">
                <img src={format!("https://gravatar.com/avatar/{}", self.user.email.hash())} />
                <div class="user-info">
                    <h3>{&self.user.name}</h3>
                    <p>{&self.user.email}</p>
                    <small>{"Joined: " + &self.user.created_at.format()}</small>
                </div>
                <button 
                    class="delete-btn"
                    onclick={move |_| (self.on_delete)(self.user.id)}
                >
                    "Delete"
                </button>
            </div>
        }
    }
}

// Application Root
#[component]
struct App;

impl Component for App {
    fn render(&self) -> Html {
        html! {
            <Router>
                <Route path="/" component={HomePage} />
                <Route path="/users" component={UserList} />
                <Route path="/users/new" component={CreateUserForm} />
                <Route path="/login" component={LoginForm} />
            </Router>
        }
    }
}

// Main entry point
#[wasm_bindgen(start)]
pub fn main() {
    console_error_panic_hook::set_once();
    
    let app = App::create();
    app.mount_to_body();
}
```

### HTML直接組み込み (ブラウザーネイティブ実行)

```html
<!DOCTYPE html>
<html lang="ja">
<head>
    <meta charset="UTF-8">
    <title>Cb App - HTML Integration</title>
    
    <!-- Cb Web Runtime -->
    <script src="https://cdn.cblang.org/cb-web-1.0.min.js"></script>
    <link rel="stylesheet" href="app.css">
</head>
<body>
    <div id="app">Loading...</div>
    
    <!-- Cb Script - 直接HTMLに記述 -->
    <script type="text/cb">
        use cb_web::{Component, html, ApiClient};
        use shared::models::User;
        
        // Simple Todo Application
        #[component]
        struct TodoApp {
            todos: Vec<Todo>,
            input_value: string,
            filter: Filter
        }
        
        struct Todo {
            id: u32,
            text: string,
            completed: bool
        }
        
        enum Filter { All, Active, Completed }
        
        impl Component for TodoApp {
            fn create() -> Self {
                Self {
                    todos: Vec::new(),
                    input_value: String::new(),
                    filter: Filter::All
                }
            }
            
            fn render(&self) -> Html {
                let filtered_todos = self.get_filtered_todos();
                
                html! {
                    <div class="todo-app">
                        <header>
                            <h1>"Cb Todo App"</h1>
                            <input 
                                type="text"
                                placeholder="What needs to be done?"
                                value={&self.input_value}
                                oninput={|e| self.update_input(e.target.value)}
                                onkeypress={|e| if e.key == "Enter" { self.add_todo() }}
                            />
                        </header>
                        
                        <main>
                            <ul class="todo-list">
                                {for todo in filtered_todos {
                                    <li class={if todo.completed { "completed" } else { "" }}>
                                        <input 
                                            type="checkbox"
                                            checked={todo.completed}
                                            onchange={move |_| self.toggle_todo(todo.id)}
                                        />
                                        <span>{&todo.text}</span>
                                        <button onclick={move |_| self.delete_todo(todo.id)}>
                                            "×"
                                        </button>
                                    </li>
                                }}
                            </ul>
                        </main>
                        
                        <footer>
                            <span>{format!("{} items left", self.active_count())}</span>
                            
                            <div class="filters">
                                <button 
                                    class={if matches!(self.filter, Filter::All) { "selected" } else { "" }}
                                    onclick={|_| self.set_filter(Filter::All)}
                                >
                                    "All"
                                </button>
                                <button 
                                    class={if matches!(self.filter, Filter::Active) { "selected" } else { "" }}
                                    onclick={|_| self.set_filter(Filter::Active)}
                                >
                                    "Active"
                                </button>
                                <button 
                                    class={if matches!(self.filter, Filter::Completed) { "selected" } else { "" }}
                                    onclick={|_| self.set_filter(Filter::Completed)}
                                >
                                    "Completed"
                                </button>
                            </div>
                        </footer>
                    </div>
                }
            }
        }
        
        impl TodoApp {
            fn add_todo(&mut self) {
                if !self.input_value.trim().is_empty() {
                    let todo = Todo {
                        id: self.next_id(),
                        text: self.input_value.clone(),
                        completed: false
                    };
                    self.todos.push(todo);
                    self.input_value.clear();
                }
            }
            
            fn toggle_todo(&mut self, id: u32) {
                if let Some(todo) = self.todos.iter_mut().find(|t| t.id == id) {
                    todo.completed = !todo.completed;
                }
            }
            
            fn delete_todo(&mut self, id: u32) {
                self.todos.retain(|t| t.id != id);
            }
            
            fn get_filtered_todos(&self) -> Vec<&Todo> {
                match self.filter {
                    Filter::All => self.todos.iter().collect(),
                    Filter::Active => self.todos.iter().filter(|t| !t.completed).collect(),
                    Filter::Completed => self.todos.iter().filter(|t| t.completed).collect()
                }
            }
            
            fn active_count(&self) -> usize {
                self.todos.iter().filter(|t| !t.completed).count()
            }
        }
        
        // Mount to DOM
        fn main() {
            let app = TodoApp::create();
            app.mount("#app");
        }
    </script>
    
    <!-- Traditional JavaScript integration -->
    <script>
        // Cb実行完了後のフック
        window.addEventListener('CbReady', () => {
            console.log('Cb Todo App loaded successfully!');
            
            // JavaScript から Cb にデータ注入
            CbRuntime.call('load_initial_data', [
                { id: 1, text: 'Learn Cb language', completed: false },
                { id: 2, text: 'Build awesome app', completed: false }
            ]);
        });
    </script>
</body>
</html>
```

## 🛠️ 技術実装詳細

### データベース統合 (CbORM)

```cb
// Database Migration
#[migration("001_create_users")]
struct CreateUsersTable;

impl Migration for CreateUsersTable {
    fn up(&self) -> String {
        "CREATE TABLE users (
            id SERIAL PRIMARY KEY,
            email VARCHAR(255) UNIQUE NOT NULL,
            name VARCHAR(100) NOT NULL,
            password_hash VARCHAR(255) NOT NULL,
            created_at TIMESTAMP DEFAULT NOW(),
            updated_at TIMESTAMP DEFAULT NOW()
        )"
    }
    
    fn down(&self) -> String {
        "DROP TABLE users"
    }
}

// Active Record Pattern
#[derive(Model)]
#[table("users")]
struct User {
    #[primary_key]
    id: i32,
    #[unique]
    email: string,
    name: string,
    #[hidden] // JSON serialize時に除外
    password_hash: string,
    created_at: DateTime,
    updated_at: DateTime
}

impl User {
    // Query Builder
    async fn find_by_email(email: &str) -> Option<Self> {
        Self::where_eq("email", email)
            .first()
            .await
            .ok()
    }
    
    // Relationship
    async fn posts(&self) -> Vec<Post> {
        Post::where_eq("user_id", self.id)
            .order_by("created_at", Order::Desc)
            .limit(10)
            .all()
            .await
            .unwrap_or_default()
    }
    
    // Validation
    fn validate(&self) -> Result<(), ValidationError> {
        if self.email.is_empty() {
            return Err(ValidationError::new("email", "Email is required"));
        }
        
        if !self.email.contains('@') {
            return Err(ValidationError::new("email", "Invalid email format"));
        }
        
        if self.name.len() < 2 {
            return Err(ValidationError::new("name", "Name too short"));
        }
        
        Ok(())
    }
}
```

### リアルタイム機能 (WebSocket/SSE)

```cb
// WebSocket Handler
#[websocket("/ws")]
struct ChatWebSocket {
    user_id: UserId,
    room_id: RoomId
}

impl WebSocketHandler for ChatWebSocket {
    async fn on_connect(&mut self, req: &Request) -> Result<(), WebSocketError> {
        self.user_id = authenticate_user(req).await?;
        self.room_id = req.query("room_id")?.parse()?;
        
        // Join room
        ChatRoom::join(self.room_id, self.user_id).await?;
        
        // Notify other users
        self.broadcast_to_room(ChatMessage::UserJoined {
            user_id: self.user_id,
            timestamp: Utc::now()
        }).await;
        
        Ok(())
    }
    
    async fn on_message(&mut self, msg: WebSocketMessage) -> Result<(), WebSocketError> {
        match msg {
            WebSocketMessage::Text(text) => {
                let chat_msg = ChatMessage::Text {
                    user_id: self.user_id,
                    text,
                    timestamp: Utc::now()
                };
                
                // Save to database
                chat_msg.save().await?;
                
                // Broadcast to room
                self.broadcast_to_room(chat_msg).await;
            },
            WebSocketMessage::Binary(data) => {
                // Handle file upload
                let file_msg = self.handle_file_upload(data).await?;
                self.broadcast_to_room(file_msg).await;
            }
        }
        Ok(())
    }
    
    async fn on_disconnect(&mut self) {
        ChatRoom::leave(self.room_id, self.user_id).await;
        self.broadcast_to_room(ChatMessage::UserLeft {
            user_id: self.user_id,
            timestamp: Utc::now()
        }).await;
    }
}
```

### 認証・認可システム

```cb
// JWT Authentication
#[middleware]
struct AuthMiddleware {
    jwt_secret: string,
    excluded_paths: Vec<string>
}

impl Middleware for AuthMiddleware {
    async fn before_request(&self, req: &mut Request) -> Result<(), MiddlewareError> {
        if self.excluded_paths.contains(&req.path) {
            return Ok(());
        }
        
        let token = req.header("Authorization")
            .and_then(|h| h.strip_prefix("Bearer "))
            .ok_or(MiddlewareError::Unauthorized)?;
            
        let claims = jwt::verify(token, &self.jwt_secret)?;
        req.set_user(User::find(claims.user_id).await?);
        
        Ok(())
    }
}

// Role-based Access Control
#[derive(Debug, Clone)]
enum Role {
    Admin,
    Moderator,
    User,
    Guest
}

#[derive(Debug, Clone)]
struct Permission {
    resource: string,
    action: string  // "read", "write", "delete", etc.
}

impl Role {
    fn has_permission(&self, permission: &Permission) -> bool {
        match self {
            Role::Admin => true,  // Admin has all permissions
            Role::Moderator => {
                permission.resource != "users" || permission.action != "delete"
            },
            Role::User => {
                permission.action == "read" || 
                (permission.resource == "posts" && permission.action == "write")
            },
            Role::Guest => permission.action == "read"
        }
    }
}

// Authorization decorator
#[authorize(permission = "posts.write")]
async fn create_post(req: Request) -> Result<Response, ApiError> {
    // This handler only runs if user has "posts.write" permission
    let user = req.user().unwrap();
    let post_data: CreatePostRequest = req.json().await?;
    
    let post = Post::create(user.id, post_data).await?;
    Ok(Response::json(post))
}
```

## 🚀 パフォーマンス最適化戦略

### サーバーサイド最適化
1. **非同期I/O**: Tokio based async runtime
2. **Connection Pooling**: データベース接続プール管理  
3. **Caching**: Redis/Memcached統合
4. **Load Balancing**: Nginx/HAProxy連携
5. **Database Optimization**: Query optimization, indexing

### クライアントサイド最適化
1. **Code Splitting**: 動的import による分割読み込み
2. **Tree Shaking**: 未使用コード削除
3. **WASM Optimization**: `wee_alloc`, サイズ最適化
4. **Service Worker**: オフライン対応、キャッシュ戦略
5. **Virtual DOM**: 効率的なDOM更新アルゴリズム

### 全体アーキテクチャ最適化
1. **CDN**: 静的ファイル配信最適化
2. **HTTP/2**: 並列リクエスト処理
3. **Compression**: Brotli/Gzip圧縮
4. **Database Sharding**: データ分散アーキテクチャ  
5. **Microservices**: サービス分割による可用性向上

## 🎯 実装ロードマップ

### Phase 9A: Web基盤構築 (3週間)
- [ ] **HTTP Server実装**: 非同期I/Oベースサーバー
- [ ] **ルーティングシステム**: RESTful API対応ルーター  
- [ ] **WebAssembly統合**: クライアントサイドWASM実行環境
- [ ] **HTML組み込み機能**: `<script type="text/cb">`サポート

### Phase 9B: フレームワーク機能 (4週間)  
- [ ] **Component System**: React-likeコンポーネントシステム
- [ ] **State Management**: 状態管理・更新システム
- [ ] **ORM実装**: データベース抽象化レイヤー
- [ ] **認証システム**: JWT・セッション管理

### Phase 9C: 開発ツール・最適化 (3週間)
- [ ] **Hot Reload**: 開発時自動更新機能
- [ ] **Build System**: プロダクションビルド最適化
- [ ] **テストフレームワーク**: 統合・単体テスト環境
- [ ] **デプロイメント**: Docker・クラウド対応

### Phase 9D: エコシステム (4週間)
- [ ] **CLI Tool**: プロジェクト生成・管理ツール
- [ ] **Package Manager**: 依存関係管理システム  
- [ ] **IDE統合**: LSP・デバッグサポート
- [ ] **Documentation**: 包括的ドキュメント・チュートリアル

## 🎯 成功指標・ベンチマーク

### パフォーマンス目標
| メトリクス | 目標値 | 比較対象 |
|----------|--------|----------|
| **Server RPS** | 50,000+ | Node.js Express (10,000) |
| **Client FPS** | 60 FPS | React (30-60 FPS) |
| **初回読み込み** | < 100ms | Next.js (200-500ms) |
| **バイナリサイズ** | < 2MB | Go (5-10MB) |
| **メモリ使用量** | < 50MB | Rails (100-200MB) |

### 開発体験指標
- [ ] **型安全性**: 100% TypeScript並み型チェック
- [ ] **Hot Reload**: < 100ms 更新レスポンス
- [ ] **ビルド時間**: < 10s (中規模プロジェクト)
- [ ] **学習コスト**: React経験者なら1週間で習得可能

### 採用指標
- [ ] **実用プロジェクト**: 5個以上のプロダクションデプロイ
- [ ] **コミュニティ**: GitHub 500+ Star, 20+ Contributors
- [ ] **企業採用**: スタートアップ3社以上での本格利用
- [ ] **技術記事**: 10+ 技術ブログ投稿・カンファレンス発表

---

**関連文書**:
- [将来展望ロードマップ](future_roadmap.md) - 全体戦略  
- [低レイヤーアプローチ](low_level_approach.md) - システムプログラミング対応
- [ブラウザーランタイム設計](browser_runtime_design.md) - HTML統合技術詳細

**更新日**: 2025年9月20日  
**ステータス**: 設計フェーズ、Phase 5-8 完了後に実装開始予定
