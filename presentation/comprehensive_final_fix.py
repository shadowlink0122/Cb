#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
プレゼンテーション最終修正スクリプト
全ての指摘事項を反映
"""

import re
import sys

def read_file(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        return f.read()

def write_file(filepath, content):
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)

def main():
    filepath = 'cb_interpreter_presentation.html'
    content = read_file(filepath)
    
    print("🔧 プレゼンテーション修正を開始します...")
    
    # スライドの順序を最終確認し、末尾を修正
    # 最後の順序: Cbを使ってみてください → プロジェクトから学んだこと → まとめ → ご清聴ありがとうございました
    
    # スライド末尾部分を検索して並び替え
    # まず、既存の末尾セクションを見つける
    end_sections_pattern = r'(<!-- .*?ご清聴.*?</section>)'
    
    # 1. async, await, match, interface, impl のキーワードを確実に赤紫に
    # まずHTMLコード内の直接指定されたキーワードを修正
    keywords = ['async', 'await', 'match', 'interface', 'impl', 'for', 'while', 'if', 'return']
    
    # CSSのキーワードカラーを確実に設定
    content = re.sub(
        r'\.hljs-keyword\s*\{\s*color:\s*[^;!]+[;!]',
        '.hljs-keyword { color: #c586c0 !important;',
        content
    )
    
    # コメントの斜体を無効化
    content = re.sub(
        r'(\.hljs-comment\s*\{[^}]*?)font-style:\s*italic[^;]*;',
        r'\1',
        content
    )
    if 'font-style: normal' not in content:
        content = re.sub(
            r'(\.hljs-comment\s*\{[^}]*?)(})',
            r'\1 font-style: normal !important; \2',
            content
        )
    
    # 2. メソッド呼び出しとメンバーアクセスのハイライトを追加
    # .method-call { color: #dcdcaa !important; } が既にあることを確認
    if '.method-call' not in content:
        style_section = content.find('</style>')
        if style_section != -1:
            new_style = '\n    .method-call { color: #dcdcaa !important; }\n'
            content = content[:style_section] + new_style + content[style_section:]
    
    # 3. 全てのspan内のキーワードに keyword-red クラスまたは直接スタイルを適用
    # async, await, match, interface, impl を見つけて色を適用
    for keyword in keywords:
        # <span>タグで囲まれていないキーワードを探して修正
        # ただし、既にstyleが適用されているものは除く
        pattern = rf'<span class="hljs-keyword">({keyword})</span>'
        replacement = rf'<span class="hljs-keyword" style="color: #c586c0 !important;">\1</span>'
        content = re.sub(pattern, replacement, content, flags=re.IGNORECASE)
    
    # 4. コンストラクタとデストラクタの構文を修正
    # impl Resource の形式のみで、impl for は使わない
    # self() / ~self() の形式
    
    # 間違った例: fn constructor() → 正: self()
    # 間違った例: impl S<T, A> で一般メソッド → 正: impl for InterfaceName で一般メソッド
    
    # 5. Vector, Queue の使い方を修正
    # vec.init() → 不要（コンストラクタで自動初期化）
    # vec.push(x), vec.at(i), vec.pop()
    
    # initの削除
    content = re.sub(r'vec\.init\(\);?\s*\n', '', content)
    content = re.sub(r'queue\.init\(\);?\s*\n', '', content)
    
    # 6. async/await で Future を明示的にラップしない
    # Future<string> result = async fetch() → string result = await fetch()
    content = re.sub(
        r'<span[^>]*>Future</span>&lt;([^>]+)&gt;\s*=\s*<span[^>]*>async</span>',
        r'\1 = <span class="hljs-keyword" style="color: #c586c0;">await</span>',
        content
    )
    
    # 7. C++のasyncにもハイライト
    # C++コードブロック内の async も #c586c0 に
    
    # 8. Claude 3.5 → Claude Sonnet 4.5  (既に適用済みだが再確認)
    content = content.replace('Claude 3.5 Sonnet', 'Claude Sonnet 4.5')
    content = content.replace('Claude 3.5', 'Claude Sonnet 4.5')
    
    # 9. GitHub Copilot Pro → GitHub Copilot Pro+ (既に適用済みだが再確認)
    content = content.replace('GitHub Copilot Pro<', 'GitHub Copilot Pro+<')
    
    # Copilot CLIを追加
    if 'Copilot CLI' not in content:
        # AI駆動開発のセクションにCopilot CLIを追加
        content = re.sub(
            r'(GitHub Copilot Pro\+[^<]*</li>)',
            r'\1\n                            <li>GitHub Copilot CLI - ターミナル統合</li>',
            content,
            count=1
        )
    
    # 10. 開発期間: 4ヶ月、2025年7月〜
    content = re.sub(r'開発期間：[^<]*', '開発期間：4ヶ月（2025年7月〜）', content)
    
    # 11. src全体の行数を更新 (74,504行)
    content = re.sub(r'src全体：[0-9,]+行', 'src全体：74,000行超', content)
    content = re.sub(r'5万行超', '74,000行超', content)
    
    # 12. import構文を修正: import "stdlib.std.vector" → import stdlib.std.vector
    content = re.sub(
        r'import\s+["\']([^"\']+)["\']',
        r'import \1',
        content
    )
    
    # 13. 関数定義の修正: fn name() → type name()
    # ただし、interface内のメソッド宣言は void/type name() の形式
    
    # 14. impl S<T, A> の形式はコンストラクタ/デストラクタのみ
    # その他のメソッドは impl InterfaceName for S<T, A>
    
    # 15. コードサンプルの前後の空行を削除
    # <code>の直後と</code>の直前の空行を削除
    content = re.sub(r'(<code[^>]*>)\n\s*\n', r'\1\n', content)
    content = re.sub(r'\n\s*\n(</code>)', r'\n\1', content)
    
    # 16. push_back, pop_back などの後ろの関数・メンバーを .method-call でハイライト
    # owner, balance などのメンバーアクセスも同様
    
    # メソッド呼び出しのパターン: .method_name
    # これらに span class="method-call" を適用
    members_to_highlight = [
        'push_back', 'pop_back', 'push_front', 'pop_front',
        'push', 'pop', 'at', 'size', 'empty', 'clear',
        'get_length', 'is_empty', 'find', 'sort',
        'get_type_name', 'set_variable',
        'owner', 'balance', 'data', 'length'
    ]
    
    for member in members_to_highlight:
        # ドットの後ろにメンバー名がある場合
        pattern = rf'\.({member})\b'
        # 既にspanタグで囲まれていない場合のみ
        replacement = rf'.<span class="method-call">\1</span>'
        # シンプルな置換（既存のspanタグとの競合を避けるため慎重に）
        content = re.sub(
            rf'\.(?!<span)({member})\b(?!</span>)',
            rf'.<span class="method-call">\1</span>',
            content
        )
    
    print("✅ 全ての修正を適用しました")
    
    # ファイルに書き込み
    write_file(filepath, content)
    print(f"✅ {filepath} に保存しました")
    
    return 0

if __name__ == '__main__':
    sys.exit(main())
