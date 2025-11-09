#!/usr/bin/env python3
"""
debug_print() を debug_msg(DebugMsgId::GENERIC_DEBUG, ...) に置き換えるスクリプト
"""

import re
import sys
from pathlib import Path

def replace_debug_print_in_file(filepath):
    """ファイル内のdebug_print呼び出しをdebug_msgに置換"""
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    original_content = content
    
    # debug_print呼び出しをすべて見つける（複数行対応）
    # 最も外側の括弧をマッチング
    pattern = r'debug_print\s*\('
    
    new_content = []
    pos = 0
    
    for match in re.finditer(pattern, content):
        # マッチ前の部分を追加
        new_content.append(content[pos:match.start()])
        
        # 括弧の対応を取って引数全体を取得
        start = match.end()
        depth = 1
        i = start
        in_string = False
        escape = False
        
        while i < len(content) and depth > 0:
            c = content[i]
            
            if escape:
                escape = False
                i += 1
                continue
            
            if c == '\\':
                escape = True
                i += 1
                continue
            
            if c == '"':
                in_string = not in_string
            elif not in_string:
                if c == '(':
                    depth += 1
                elif c == ')':
                    depth -= 1
            
            i += 1
        
        if depth != 0:
            # 括弧が閉じていない場合はそのまま
            new_content.append(content[match.start():i])
            pos = i
            continue
        
        # 引数全体を取得
        args_str = content[start:i-1].strip()
        
        # セミコロンの後までをスキップ
        semicolon_pos = i
        while semicolon_pos < len(content) and content[semicolon_pos] not in ';\n':
            semicolon_pos += 1
        if semicolon_pos < len(content) and content[semicolon_pos] == ';':
            semicolon_pos += 1
        
        # 文字列部分と引数部分を分離
        # 最初の文字列リテラルを探す
        string_match = re.match(r'"((?:[^"\\]|\\.)*)"(.*)$', args_str, re.DOTALL)
        
        if string_match:
            format_str = string_match.group(1)
            remaining_args = string_match.group(2).strip()
            
            # \n を削除
            format_str_clean = format_str.replace('\\n', '')
            
            # 引数があるかチェック
            if remaining_args.startswith(','):
                # 引数がある場合
                args_part = remaining_args
                replacement = (
                    f'{{ char dbg_buf[512]; '
                    f'snprintf(dbg_buf, sizeof(dbg_buf), "{format_str_clean}"{args_part}); '
                    f'debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf); }}'
                )
            else:
                # 引数がない場合
                replacement = f'debug_msg(DebugMsgId::GENERIC_DEBUG, "{format_str_clean}");'
        else:
            # パースできない場合はそのまま
            replacement = content[match.start():semicolon_pos]
        
        new_content.append(replacement)
        pos = semicolon_pos
    
    # 残りの部分を追加
    new_content.append(content[pos:])
    
    result = ''.join(new_content)
    
    # 変更があれば書き込み
    if result != original_content:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(result)
        # 変更数をカウント
        changes = result.count('debug_msg(DebugMsgId::GENERIC_DEBUG') - original_content.count('debug_msg(DebugMsgId::GENERIC_DEBUG')
        return True, changes
    
    return False, 0

def main():
    src_dir = Path('src')
    
    if not src_dir.exists():
        print("Error: src/ directory not found", file=sys.stderr)
        return 1
    
    total_files = 0
    total_changes = 0
    
    # .cpp と .h ファイルを処理
    for pattern in ['**/*.cpp', '**/*.h']:
        for filepath in src_dir.glob(pattern):
            # debug_impl.cpp はスキップ（debug_print関数の定義がある）
            if filepath.name == 'debug_impl.cpp':
                continue
            
            # debug.h もスキップ（宣言がある）
            if filepath.name == 'debug.h':
                continue
            
            changed, num_changes = replace_debug_print_in_file(filepath)
            if changed:
                total_files += 1
                total_changes += num_changes
                print(f"Updated: {filepath} ({num_changes} replacements)")
    
    print(f"\nTotal: {total_files} files updated, {total_changes} replacements made")
    return 0

if __name__ == '__main__':
    sys.exit(main())
