#!/usr/bin/env python3
"""最終スライドの順序を修正"""
import re

with open('cb_interpreter_presentation.html', 'r', encoding='utf-8') as f:
    content = f.read()

# 「インタプリタからコンパイラへ」セクション全体を削除
compiler_section = re.search(
    r'<!-- スライド22: コンパイラ化への道 -->.*?</section>\s*\n',
    content,
    re.DOTALL
)

if compiler_section:
    content = content[:compiler_section.start()] + content[compiler_section.end():]
    print("削除: インタプリタからコンパイラへ")

# 「WebAssembly対応の展望」セクション全体を削除
wasm_section = re.search(
    r'<!-- スライド23: WebAssembly対応 -->.*?</section>\s*\n',
    content,
    re.DOTALL
)

if wasm_section:
    content = content[:wasm_section.start()] + content[wasm_section.end():]
    print("削除: WebAssembly対応の展望")

# まとめセクションの位置を確認
summary_match = re.search(r'(<!-- スライド24: まとめ -->.*?</section>)', content, re.DOTALL)
learned_match = re.search(r'(<!-- 新スライド: 学んだこと -->.*?</section>)', content, re.DOTALL)

if summary_match and learned_match:
    # 順序が逆なので入れ替え
    if summary_match.start() < learned_match.start():
        summary_section = summary_match.group(1)
        learned_section = learned_match.group(1)
        
        # まとめセクションを削除
        content = content[:summary_match.start()] + content[summary_match.end():]
        
        # 学んだことセクションの後に挿入
        learned_match = re.search(r'(<!-- 新スライド: 学んだこと -->.*?</section>)', content, re.DOTALL)
        if learned_match:
            insert_pos = learned_match.end()
            content = content[:insert_pos] + '\n\n            ' + summary_section + content[insert_pos:]
            print("順序変更: プロジェクトから学んだこと → まとめ")

with open('cb_interpreter_presentation.html', 'w', encoding='utf-8') as f:
    f.write(content)

print("✓ 最終スライドの順序を修正しました")
