#!/usr/bin/env python3
import re
import sys

def remove_html_tags_from_code(content):
    """Remove HTML span tags from code blocks while preserving structure"""
    
    # Pattern to match code blocks
    code_pattern = r'(<code[^>]*>)(.*?)(</code>)'
    
    def clean_code_block(match):
        opening = match.group(1)
        code_content = match.group(2)
        closing = match.group(3)
        
        # Remove all span tags
        code_content = re.sub(r'<span[^>]*>', '', code_content)
        code_content = re.sub(r'</span>', '', code_content)
        
        return opening + code_content + closing
    
    result = re.sub(code_pattern, clean_code_block, content, flags=re.DOTALL)
    return result

def main():
    if len(sys.argv) < 2:
        print("Usage: python fix_html_in_code.py <file>")
        sys.exit(1)
    
    filepath = sys.argv[1]
    
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    cleaned = remove_html_tags_from_code(content)
    
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(cleaned)
    
    print(f"Fixed: {filepath}")

if __name__ == '__main__':
    main()
