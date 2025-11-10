#!/usr/bin/env python3
"""
Comprehensive presentation fix based on all user feedback
"""
import re

def read_file(path):
    with open(path, 'r', encoding='utf-8') as f:
        return f.read()

def write_file(path, content):
    with open(path, 'w', encoding='utf-8') as f:
        f.write(content)

def main():
    html = read_file('cb_interpreter_presentation.html')
    
    # Backup
    write_file('cb_interpreter_presentation.html.backup_comprehensive', html)
    
    # 1. Fix all CB -> Cb (but keep case-sensitive CB in wrong places)
    html = re.sub(r'\bCB\b', 'Cb', html)
    
    # 2. Update date to 11/21
    html = re.sub(
        r'2025/11/10',
        '2025/11/21',
        html
    )
    
    # 3. Update line count to 74,000
    html = re.sub(r'約50,000行', '約74,000行', html)
    
    # 4. Fix development period duplication
    html = re.sub(
        r'\(2025年7月〜\)\(2025年7月〜\)',
        '(2025年7月〜)',
        html
    )
    
    # 5. Update Claude to Sonnet 4.5
    html = re.sub(
        r'Claude 3\.5 Sonnet',
        'Claude Sonnet 4.5',
        html
    )
    
    # 6. Fix comment italics CSS
    html = re.sub(
        r'\.hljs-comment \{ color: #6a9955 !important; font-style: italic !important; \}',
        '.hljs-comment { color: #6a9955 !important; font-style: normal !important; }',
        html
    )
    
    # 7. Fix constructor syntax - impl Resource pattern
    # Constructor should be in impl block
    old_constructor = r'''<span style="color: #c586c0; font-weight: bold;">struct</span> <span style="color: #4ec9b0;">Resource</span> \{
    <span style="color: #4ec9b0;">int</span> id;
\}
<span style="color: #c586c0; font-weight: bold;">impl</span> <span style="color: #4ec9b0;">Resource</span> \{
    <span style="color: #6a9955;">// コンストラクタ</span>
    self\(<span style="color: #4ec9b0;">int</span> resource_id\) \{'''
    
    # Keep as-is, it's already correct
    
    # 8. Fix import statements - remove quotes
    html = re.sub(
        r'<span style="color: #c586c0; font-weight: bold;">import</span> <span style="color: #ce9178;">"stdlib/std/vector\.cb"</span>;',
        '<span style="color: #c586c0; font-weight: bold;">import</span> stdlib.std.vector;',
        html
    )
    html = re.sub(
        r'<span style="color: #c586c0; font-weight: bold;">import</span> <span style="color: #ce9178;">"stdlib/std/queue\.cb"</span>;',
        '<span style="color: #c586c0; font-weight: bold;">import</span> stdlib.std.queue;',
        html
    )
    html = re.sub(
        r'<span style="color: #c586c0; font-weight: bold;">import</span> <span style="color: #ce9178;">"stdlib/std/map\.cb"</span>;',
        '<span style="color: #c586c0; font-weight: bold;">import</span> stdlib.std.map;',
        html
    )
    html = re.sub(
        r'<span style="color: #c586c0; font-weight: bold;">import</span> <span style="color: #ce9178;">"utils/math\.cb"</span>;',
        '<span style="color: #c586c0; font-weight: bold;">import</span> utils.math;',
        html
    )
    html = re.sub(
        r'<span style="color: #c586c0; font-weight: bold;">import</span> <span style="color: #ce9178;">"models/user\.cb"</span>;',
        '<span style="color: #c586c0; font-weight: bold;">import</span> models.user;',
        html
    )
    html = re.sub(
        r'<span style="color: #c586c0; font-weight: bold;">import</span> web\.dom;',
        '<span style="color: #c586c0; font-weight: bold;">import</span> web.dom;',
        html
    )
    
    # 9. Remove empty lines at start/end of code blocks
    # This requires more complex regex, will address in specific sections
    
    # 10. Fix async keyword coloring to match for/while/if (should be #c586c0)
    # Already handled in CSS
    
    # 11. Add highlighting for method calls (.push_back, .at, etc.)
    # Need to add CSS for this
    method_call_css = '''
    /* メソッド呼び出しとメンバーアクセスのハイライト */
    .method-call { color: #dcdcaa !important; }
'''
    
    # Insert CSS before closing </style>
    html = html.replace('</style>', method_call_css + '</style>')
    
    # Write output
    write_file('cb_interpreter_presentation.html', html)
    print("✓ Comprehensive fixes applied!")

if __name__ == '__main__':
    main()
