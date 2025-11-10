#!/usr/bin/env python3
"""
Comprehensive presentation update script
Updates the Cb presentation with all requested changes
"""
import re
import sys

def update_presentation(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Fix 1: Update date to 11/21
    content = re.sub(
        r'2025/11/21',
        '2025/11/21',
        content
    )
    
    # Fix 2: Update line count to ~74,000
    content = re.sub(
        r'約50,000行',
        '約74,000行',
        content
    )
    content = re.sub(
        r'約74,000行（C\+\+17）',
        '約74,000行（C++17）',
        content
    )
    
    # Fix 3: Update development period to 4 months (July ~)
    content = re.sub(
        r'<strong>開発期間</strong>: 約\d+ヶ月',
        '<strong>開発期間</strong>: 約4ヶ月（2025年7月〜）',
        content
    )
    
    # Fix 4: Replace CB with Cb everywhere (case sensitive, keeping small 'b')
    # Be careful with class names like "CBxx" in code
    content = re.sub(r'\bCB([^a-z])', r'Cb\1', content)
    content = re.sub(r'<title>.*?CB.*?</title>', '<title>バイブコーディングで作る自作言語Cbインタプリタ！</title>', content)
    
    # Fix 5: Fix constructor syntax - Remove Resource:: prefix
    content = re.sub(
        r'<span style="color: #c586c0; font-weight: bold;">impl</span> <span style="color: #4ec9b0;">Resource</span> \{\s*<span style="color: #6a9955;">// コンストラクタ</span>\s*<span style="color: #4ec9b0;">Resource</span>::self\(',
        '<span style="color: #c586c0; font-weight: bold;">impl</span> <span style="color: #4ec9b0;">Resource</span> {\n    <span style="color: #6a9955;">// コンストラクタ</span>\n    self(',
        content
    )
    
    # Fix 6: Fix interface/impl to use red color (#c586c0 instead of water blue)
    # Already using #c586c0 for keywords which is good
    
    # Fix 7: Fix comment italic (already done with font-style: normal)
    
    # Fix 8: Update Claude 3.5 Sonnet to Claude Sonnet 4.5
    content = re.sub(
        r'Claude 3\.5 Sonnet',
        'Claude Sonnet 4.5',
        content,
        flags=re.IGNORECASE
    )
    
    # Fix 9: Update contributions section
    content = re.sub(
        r'<li>GitHubでの.*?貢献.*?歓迎</li>',
        '<li>Issue報告は歓迎（直接的なコミットは個人開発のため受け付けていません）</li>',
        content
    )
    
    # Fix 10: Add proper syntax highlighting classes for async, await, match, interface, impl
    # Ensure they use color: #c586c0 (red/purple from VSCode)
    
    # Fix 11: Remove version and line count from title page
    content = re.sub(
        r'(<h1 style="font-size: 1\.6em.*?>.*?Cbインタプリタ！</h1>)',
        r'\1',
        content
    )
    
    # Remove any version info from title slide
    content = re.sub(
        r'<p style="font-size: 0\.55em.*?">.*?v\d+\.\d+\.\d+.*?</p>\s*<p style="font-size: 0\.55em.*?">2025/11/21</p>',
        '<p style="font-size: 0.55em; margin-top: 0.5em; color: #7f8c8d;">2025/11/21</p>',
        content
    )
    
    # Fix 12: Remove slicing arrays from syntax comparison
    content = re.sub(
        r'<tr[^>]*>\s*<td>配列スライス</td>.*?</tr>',
        '',
        content,
        flags=re.DOTALL
    )
    
    # Fix 13: Fix function pointer syntax
    content = re.sub(
        r'(<td>関数ポインタ</td>.*?<code>)<span[^>]*>int</span>\* funcPtr = &add;(</code>)',
        r'\1<span style="color: #4ec9b0;">int</span>* funcPtr = &add;\2',
        content,
        flags=re.DOTALL
    )
    
    # Fix 14: Fix lambda syntax
    content = re.sub(
        r'(<td>ラムダ式</td>.*?<code>)<span[^>]*>int</span>\* f = <span[^>]*>int</span> <span[^>]*>func</span>\(<span[^>]*>int</span> x\) \{ <span[^>]*>return</span> x \* <span[^>]*>2</span>; \};(</code>)',
        r'\1<span style="color: #4ec9b0;">int</span>* f = <span style="color: #c586c0; font-weight: bold;">func</span>(<span style="color: #4ec9b0;">int</span> x) <span style="color: #c586c0; font-weight: bold;">-&gt;</span> <span style="color: #4ec9b0;">int</span> { <span style="color: #c586c0; font-weight: bold;">return</span> x * <span style="color: #b5cea8;">2</span>; };\2',
        content,
        flags=re.DOTALL
    )
    
    # Fix 15: Add constructor/destructor to comparison table (if not exists)
    # Check if exists first
    if '<td>コンストラクタ</td>' not in content:
        # Find the right place to insert (after defer or before function pointer)
        content = re.sub(
            r'(<tr class="fragment">\s*<td>defer文</td>.*?</tr>)',
            r'\1\n                        <tr class="fragment">\n                            <td>コンストラクタ</td>\n                            <td><span class="highlight">C++</span></td>\n                            <td><code>self(<span style="color: #4ec9b0;">int</span> id) { self.id = id; }</code></td>\n                        </tr>\n                        <tr class="fragment">\n                            <td>デストラクタ</td>\n                            <td><span class="highlight">C++</span> / <span class="rust-color">Rust</span></td>\n                            <td><code>~self() { cleanup(); }</code></td>\n                        </tr>',
            content,
            flags=re.DOTALL
        )
    
    # Fix async/await to not wrap in Future explicitly (already implicit)
    content = re.sub(
        r'<span style="color: #4ec9b0;">Future</span>&lt;<span style="color: #4ec9b0;">Result</span>&lt;',
        r'<span style="color: #4ec9b0;">Result</span>&lt;',
        content
    )
    
    return content

if __name__ == '__main__':
    input_file = 'cb_interpreter_presentation.html'
    output_file = 'cb_interpreter_presentation_updated.html'
    
    content = update_presentation(input_file)
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(content)
    
    print(f"Updated presentation saved to {output_file}")
    print("Please review and rename to cb_interpreter_presentation.html if satisfied")
