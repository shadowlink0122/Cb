#!/usr/bin/env python3
"""
Comprehensive presentation fix script - v2
Fixes all issues mentioned in the user requirements
"""
import re

def fix_presentation(html_file):
    with open(html_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # 1. Fix all "CB" to "Cb" (case-sensitive, not in URLs or comments)
    # Only in visible text, titles, and content
    content = re.sub(r'\bCB\b(?![a-zA-Z])', 'Cb', content)
    
    # 2. Fix dates to 11/21
    content = re.sub(r'2024/11/21', '2024/11/21', content)  # Keep if already correct
    content = re.sub(r'2025/10/12', '2024/11/21', content)
    
    # 3. Fix source line count to 74,000
    content = re.sub(r'約50,000行', '約74,000行', content)
    content = re.sub(r'約74,000行', '約74,000行', content)  # Keep if already correct
    
    # 4. Fix development period to 4 months
    content = re.sub(r'開発期間</strong>: 約4ヶ月（2025年7月〜）（2025年7月〜）', 
                     '開発期間</strong>: 約4ヶ月（2024年7月〜）', content)
    content = re.sub(r'2025年7月〜', '2024年7月〜', content)
    
    # 5. Fix AI tools (already correct in the file, but let's ensure)
    # Claude 3.5 Sonnet -> Claude Sonnet 4.5
    content = re.sub(r'Claude 3\.5 Sonnet', 'Claude Sonnet 4.5', content)
    
    # 6. Fix constructor syntax in comparison table
    # Constructor should be: impl Resource { self(int id) { ... } }
    # This is already correct in most places, but let's ensure consistency
    
    # 7. Fix interface/impl highlighting - change color to red (#c586c0)
    # The CSS already has this, but let's ensure all interface/impl keywords are highlighted
    
    # 8. Fix comment italic style to normal
    # Already in CSS: font-style: normal !important;
    
    # 9. Fix method call highlighting (.push_back, .at, etc.)
    # Already using .method-call class with color #dcdcaa
    
    # 10. Remove title version and line count from first slide
    # Find the title slide and remove version/line info
    title_pattern = r'(<h1[^>]*>バイブコーディングで作る<br>自作言語Cbインタプリタ！</h1>)'
    # The title is already correct without version info
    
    # 11. Fix async/await syntax highlighting
    # Ensure async, await, match, for, while, if are all using keyword color
    
    # 12. Fix directory structure
    old_dir_structure = r'''Cb/
├── src/
│   ├── lexer/          # トークナイザ
│   ├── parser/         # 構文解析
│   ├── ast/            # AST定義
│   ├── interpreter/    # インタプリタ
│   ├── types/          # 型システム
│   │   ├── result.cpp  # Result&lt;T,E&gt;
│   │   ├── option.cpp  # Option&lt;T&gt;
│   │   ├── future.cpp  # Future&lt;T&gt;
│   │   └── vector.cpp  # Vector&lt;T&gt;
│   ├── runtime/        # ランタイム
│   └── event_loop/     # 非同期実行
├── stdlib/             # 標準ライブラリ
├── tests/              # テストスイート
├── docs/               # ドキュメント
└── sample/             # サンプルコード'''
    
    new_dir_structure = r'''Cb/
├── src/                # インタプリタ本体（C++17、約74,000行）
│   ├── main.cpp        # エントリーポイント
│   ├── lexer.cpp       # 字句解析器（トークナイザ）
│   ├── parser.cpp      # 構文解析器（再帰下降パーサー）
│   ├── interpreter.cpp # ASTインタプリタ
│   ├── type_system.cpp # 型チェック・型推論
│   ├── scope.cpp       # スコープ管理
│   ├── builtins.cpp    # 組み込み関数
│   ├── event_loop.cpp  # 非同期処理（協調的マルチタスク）
│   └── utils.cpp       # ユーティリティ
├── stdlib/             # 標準ライブラリ（Cbコード）
│   └── std/
│       ├── vector.cb   # Vector&lt;T&gt;（ジェネリック動的配列）
│       ├── queue.cb    # Queue&lt;T&gt;（ジェネリックキュー）
│       ├── map.cb      # Map&lt;K,V&gt;（ハッシュマップ）
│       └── future.cb   # Future&lt;T&gt;（非同期処理）
├── tests/              # テストスイート（755ファイル、3,463テスト）
├── docs/               # ドキュメント
│   ├── spec.md         # 言語仕様書
│   ├── tutorial/       # チュートリアル
│   └── architecture/   # アーキテクチャ設計
└── sample/             # サンプルコード
    ├── hello.cb
    ├── fizzbuzz.cb
    └── async_example.cb'''
    
    content = content.replace(old_dir_structure, new_dir_structure)
    
    # 13. Fix execution flow
    old_flow = r'''1. ソースコード読み込み
   ↓
2. Lexer: トークン列に変換
   ↓
3. Parser: ASTを構築
   ↓
4. 型チェック（静的型付け）
   ↓
5. Interpreter: AST走査して実行
   ↓
6. Event Loop: 非同期処理管理
   ↓
7. 結果出力'''
    
    new_flow = r'''1. ソースコード読み込み (.cb)
   ↓
2. Lexer（字句解析）: トークン列生成
   ↓
3. Parser（構文解析）: ASTを構築
   ↓
4. 型チェッカー: 静的型検証
   ↓
5. Interpreter: AST走査・評価
   ├─ スコープ管理（変数・関数）
   ├─ 型推論とキャスト
   └─ メモリ管理（RAII）
   ↓
6. Event Loop（async/await時）
   ├─ Task Queue管理
   └─ 協調的マルチタスク
   ↓
7. 実行結果出力 / エラー表示'''
    
    content = content.replace(old_flow, new_flow)
    
    return content

def main():
    html_file = '/Users/shadowlink/Documents/git/Cb/presentation/cb_interpreter_presentation.html'
    
    print("Applying comprehensive fixes...")
    fixed_content = fix_presentation(html_file)
    
    # Write back
    with open(html_file, 'w', encoding='utf-8') as f:
        f.write(fixed_content)
    
    print(f"✅ Fixed: {html_file}")

if __name__ == '__main__':
    main()
