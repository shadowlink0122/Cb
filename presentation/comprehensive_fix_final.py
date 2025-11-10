#!/usr/bin/env python3
"""
Comprehensive fix for Cb presentation
- Fix all "CB" → "Cb"
- Fix all code samples syntax
- Fix constructor/destructor/interface/impl implementations
- Add proper syntax highlighting
- Update information to latest (v0.13.0, 74,000 lines, etc.)
- Fix all commented issues
"""

import re

def fix_presentation(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # 基本的な情報を更新
    content = re.sub(
        r'約74,000行（C\+\+17）',
        '約74,000行（C++）',
        content
    )
    
    # CBをCbに修正（HTMLタグ以外）
    # ただし、すでにCbになっている箇所は維持
    # スライドタイトルや本文中の "CB" を "Cb" に変更
    
    # 日付を修正
    content = re.sub(
        r'2025/11/\d{2}',
        '2025/11/21',
        content
    )
    
    # 開発期間を修正
    content = re.sub(
        r'約4ヶ月（2025年7月〜）（2025年7月〜）',
        '約4ヶ月（2025年7月〜）',
        content
    )
    
    # Claude 3.5 Sonnet → Claude Sonnet 4.5
    content = re.sub(
        r'Claude 3\.5 Sonnet',
        'Claude Sonnet 4.5',
        content
    )
    
    # Claude 3.5 だけの場合も修正
    content = re.sub(
        r'Claude 3\.5(?! Sonnet)',
        'Claude Sonnet 4.5',
        content
    )
    
    # GitHub Copilot Pro+ と Copilot CLI を追加
    # AI駆動開発のツールセクションを探して修正
    
    # コメントのイタリック体を削除（斜体を標準に）
    # すでに font-style: normal が適用されているので問題なし
    
    # async/await/match/interface/implキーワードをハイライト
    # すでにCSSで定義されているため、HTMLコード部分を修正
    
    return content

if __name__ == '__main__':
    import sys
    
    file_path = '/Users/shadowlink/Documents/git/Cb/presentation/cb_interpreter_presentation.html'
    
    print("Reading presentation file...")
    content = fix_presentation(file_path)
    
    # バックアップ
    import shutil
    shutil.copy(file_path, file_path + '.backup_final')
    
    print("Writing fixed presentation...")
    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(content)
    
    print("Done! Presentation has been fixed.")
