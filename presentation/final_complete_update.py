#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
最終的なプレゼンテーション修正スクリプト
全ての修正を一括で適用
"""

import re

def read_file(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        return f.read()

def write_file(filepath, content):
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)

def fix_presentation(content):
    """全ての修正を適用"""
    
    # 1. タイトルページの修正（行数とバージョンを削除、日付を2025/11/21に）
    content = re.sub(
        r'<section>\s*<h1[^>]*>バイブコーディングで作る<br>自作言語C[bB]インタプリタ！</h1>.*?</section>',
        '''<section>
                <h1 style="font-size: 1.6em; line-height: 1.1; margin-top: 1em;">バイブコーディングで作る<br>自作言語Cbインタプリタ！</h1>
                <p style="font-size: 0.7em; margin-top: 0.8em;">AI駆動開発で実現する理想の言語 - 他言語のいいとこ取り</p>
                <p style="font-size: 0.6em; margin-top: 1.5em;">miyajima (@sl_0122)</p>
                <p style="font-size: 0.55em; margin-top: 0.5em; color: #7f8c8d;">2025/11/21</p>
            </section>''',
        content,
        flags=re.DOTALL
    )
    
    # 2. 全てのCBをCbに修正（HTML内の全箇所）
    content = re.sub(r'\bCB\b', 'Cb', content)
    
    # 3. コメントの斜体を無効化（既存の設定を確認・強化）
    if '.hljs-comment' not in content or 'font-style: normal' not in content:
        content = re.sub(
            r'(\.hljs-comment\s*{[^}]*)(})',
            r'\1 font-style: normal !important; \2',
            content
        )
    
    # 4. 開発期間を4ヶ月に修正
    content = re.sub(r'開発期間：\s*\d+年\d+ヶ月', '開発期間：4ヶ月（2025年7月〜）', content)
    content = re.sub(r'開発期間：.*?(?=<)', '開発期間：4ヶ月（2025年7月〜）', content)
    
    # 5. Claude 3.5 SonnetをClaude Sonnet 4.5に修正
    content = content.replace('Claude 3.5 Sonnet', 'Claude Sonnet 4.5')
    content = content.replace('Claude 3.5', 'Claude Sonnet 4.5')
    
    # 6. GitHub Copilot Proを GitHub Copilot Pro+ に修正
    content = content.replace('GitHub Copilot Pro', 'GitHub Copilot Pro+')
    
    # 7. async/await/match/interface/implのハイライトを#c586c0（赤紫）に統一
    # CSSスタイルを更新
    content = re.sub(
        r'\.hljs-keyword\s*{\s*color:\s*[^;]+;',
        '.hljs-keyword { color: #c586c0 !important;',
        content
    )
    
    return content

def main():
    input_file = '/Users/shadowlink/Documents/git/Cb/presentation/cb_interpreter_presentation.html'
    
    print("プレゼンテーションファイルを読み込み中...")
    content = read_file(input_file)
    
    print("修正を適用中...")
    content = fix_presentation(content)
    
    print("ファイルに書き込み中...")
    write_file(input_file, content)
    
    print("✓ 修正完了！")

if __name__ == '__main__':
    main()
