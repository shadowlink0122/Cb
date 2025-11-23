#!/usr/bin/env python3
"""
Convert format specifiers (%d, %s, etc.) to expression embedding {VAR}
"""
import re
import sys
from pathlib import Path

def convert_line(line):
    """Convert a single line from format specifiers to expression embedding"""
    # Pattern for println/print with format specifiers
    # Matches: println("format %d %s", arg1, arg2);
    pattern = r'(println?)\s*\(\s*"([^"]*?)"\s*,\s*([^;)]+)\s*\)'
    
    def replace_format(match):
        func_name = match.group(1)
        format_str = match.group(2)
        args_str = match.group(3)
        
        # Check if there are format specifiers
        if '%' not in format_str:
            return match.group(0)
        
        # Split arguments, handling nested function calls and commas
        args = []
        depth = 0
        current = ""
        for char in args_str:
            if char == '(' or char == '[':
                depth += 1
                current += char
            elif char == ')' or char == ']':
                depth -= 1
                current += char
            elif char == ',' and depth == 0:
                args.append(current.strip())
                current = ""
            else:
                current += char
        if current.strip():
            args.append(current.strip())
        
        # Find all format specifiers
        spec_pattern = r'%(?:\d+)?([dsfcp]|lld)'
        specs = list(re.finditer(spec_pattern, format_str))
        
        if not specs:
            return match.group(0)
        
        if len(specs) > len(args):
            # More format specs than args - skip conversion
            return match.group(0)
        
        # Replace format specifiers with {arg}
        result = format_str
        offset = 0
        for i, spec_match in enumerate(specs):
            if i >= len(args):
                break
            old_spec = spec_match.group(0)
            new_spec = '{' + args[i] + '}'
            start = spec_match.start() + offset
            end = spec_match.end() + offset
            result = result[:start] + new_spec + result[end:]
            offset += len(new_spec) - len(old_spec)
        
        # If we used all args in the format string, no trailing args
        if len(specs) == len(args):
            return f'{func_name}("{result}")'
        else:
            # There are extra args beyond format specs
            remaining_args = ', '.join(args[len(specs):])
            return f'{func_name}("{result}", {remaining_args})'
    
    return re.sub(pattern, replace_format, line)

def convert_file(filepath):
    """Convert a single file"""
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
        
        lines = content.split('\n')
        converted_lines = [convert_line(line) for line in lines]
        new_content = '\n'.join(converted_lines)
        
        if new_content != content:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(new_content)
            return True
        return False
    except Exception as e:
        print(f"Error processing {filepath}: {e}", file=sys.stderr)
        return False

def main():
    if len(sys.argv) > 1:
        # Process specific files
        for filepath in sys.argv[1:]:
            if convert_file(filepath):
                print(f"Converted: {filepath}")
    else:
        # Process all .cb files in tests directory
        tests_dir = Path('tests')
        if not tests_dir.exists():
            print("tests directory not found", file=sys.stderr)
            sys.exit(1)
        
        count = 0
        for cb_file in tests_dir.rglob('*.cb'):
            if convert_file(cb_file):
                print(f"Converted: {cb_file}")
                count += 1
        
        print(f"\nTotal files converted: {count}")

if __name__ == '__main__':
    main()
