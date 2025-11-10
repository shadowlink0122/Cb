#!/usr/bin/env python3
"""
Comprehensive presentation update script
Updates all aspects of the Cb presentation based on user feedback
"""

import re
import sys

def read_file(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        return f.read()

def write_file(filepath, content):
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)

def update_presentation(html_content):
    """Apply all comprehensive updates to the presentation"""
    
    # 1. Update all "CB" to "Cb" (case-sensitive)
    html_content = re.sub(r'\bCB\b', 'Cb', html_content)
    
    # 2. Update version date to 11/21
    html_content = re.sub(
        r'<p style="font-size: 0\.55em; margin-top: 0\.5em; color: #7f8c8d;">.*?</p>',
        '<p style="font-size: 0.55em; margin-top: 0.5em; color: #7f8c8d;">2025/11/21</p>',
        html_content
    )
    
    # 3. Update line count to 74,409
    html_content = re.sub(
        r'約[\d,]+行',
        '約74,000行',
        html_content
    )
    html_content = re.sub(
        r'約50,000行',
        '約74,000行',
        html_content
    )
    
    # 4. Remove version/line count from title page
    html_content = re.sub(
        r'(<h1 style="font-size: 1\.6em[^>]*>バイブコーディングで作る<br>自作言語Cbインタプリタ！</h1>)',
        r'\1',
        html_content
    )
    
    # 5. Update AI tool to Claude Sonnet 4.5
    html_content = re.sub(
        r'Claude 3\.5 Sonnet',
        'Claude Sonnet 4.5',
        html_content
    )
    
    # 6. Update development period to 4 months
    html_content = re.sub(
        r'開発期間</strong>: 約[\d]+ヶ月',
        '開発期間</strong>: 約4ヶ月',
        html_content
    )
    html_content = re.sub(
        r'\(2025年7月〜\)\(2025年7月〜\)',
        '(2025年7月〜)',
        html_content
    )
    
    # 7. Fix syntax highlighting for interface/impl/async/await/match
    # Make them red (#c586c0) like other keywords
    html_content = re.sub(
        r'<span style="color: #c586c0; font-weight: bold;">interface</span>',
        '<span class="hljs-keyword" style="color: #c586c0 !important;">interface</span>',
        html_content
    )
    html_content = re.sub(
        r'<span style="color: #c586c0; font-weight: bold;">impl</span>',
        '<span class="hljs-keyword" style="color: #c586c0 !important;">impl</span>',
        html_content
    )
    
    # 8. Fix comment italics (make them not italic)
    html_content = re.sub(
        r'\.hljs-comment \{ color: #6a9955 !important; font-style: italic !important; \}',
        '.hljs-comment { color: #6a9955 !important; font-style: normal !important; }',
        html_content
    )
    
    # 9. Add method call highlighting (.push_back, .at, etc.)
    # This is handled in CSS
    
    # 10. Fix import statement format (remove quotes)
    html_content = re.sub(
        r'<span style="color: #c586c0; font-weight: bold;">import</span> <span style="color: #ce9178;">"([^"]+)"</span>;',
        r'<span style="color: #c586c0; font-weight: bold;">import</span> \1;',
        html_content
    )
    
    # 11. Update community section
    html_content = re.sub(
        r'issueは歓迎していますが直接的なコミットは歓迎していません',
        'issueは歓迎していますが、直接的なコミットは歓迎していません。理由は自分が作りたいからです',
        html_content
    )
    
    return html_content

def main():
    input_file = 'cb_interpreter_presentation.html'
    output_file = 'cb_interpreter_presentation.html'
    backup_file = 'cb_interpreter_presentation.html.backup3'
    
    print(f"Reading {input_file}...")
    html_content = read_file(input_file)
    
    print("Creating backup...")
    write_file(backup_file, html_content)
    
    print("Applying comprehensive updates...")
    updated_content = update_presentation(html_content)
    
    print(f"Writing to {output_file}...")
    write_file(output_file, updated_content)
    
    print("✓ Presentation updated successfully!")
    print(f"  - Backup saved to {backup_file}")
    print(f"  - Updated file: {output_file}")

if __name__ == '__main__':
    main()
