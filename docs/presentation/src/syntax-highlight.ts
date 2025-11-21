/**
 * Simple syntax highlighter for Cb language
 */

export function highlightCbCode(): void {
    // Find all code blocks with language-cb class
    const codeBlocks = document.querySelectorAll('code.language-cb');

    codeBlocks.forEach(block => {
        let html = block.innerHTML;

        // Preserve original spacing and newlines
        const originalHtml = html;

        // Keywords
        const keywords = [
            'if', 'else', 'for', 'while', 'do', 'switch', 'case', 'default', 'break', 'continue',
            'return', 'void', 'const', 'static', 'struct', 'enum', 'typedef', 'interface', 'impl',
            'async', 'await', 'try', 'catch', 'finally', 'throw', 'new', 'delete', 'this',
            'template', 'typename', 'when', 'foreign', 'module', 'import', 'export'
        ];

        // Types
        const types = [
            'int', 'long', 'short', 'tiny', 'char', 'bool', 'float', 'double', 'string',
            'void', 'auto', 'Future', 'Option', 'Result', 'Some', 'None', 'Ok', 'Err'
        ];

        // Built-in functions
        const functions = [
            'println', 'print', 'malloc', 'free', 'memcpy', 'sizeof', 'sleep'
        ];

        // Temporary placeholders for strings and comments to avoid replacing inside them
        const placeholders: string[] = [];
        let placeholderIndex = 0;

        // Replace strings first
        html = html.replace(/"[^"]*"/g, (match) => {
            const placeholder = `__STRING_${placeholderIndex++}__`;
            placeholders.push(`<span class="string">${match}</span>`);
            return placeholder;
        });

        // Replace single-line comments
        html = html.replace(/\/\/[^\n]*/g, (match) => {
            const placeholder = `__COMMENT_${placeholderIndex++}__`;
            placeholders.push(`<span class="comment">${match}</span>`);
            return placeholder;
        });

        // Replace multi-line comments
        html = html.replace(/\/\*[\s\S]*?\*\//g, (match) => {
            const placeholder = `__COMMENT_${placeholderIndex++}__`;
            placeholders.push(`<span class="comment">${match}</span>`);
            return placeholder;
        });

        // Replace preprocessor directives
        html = html.replace(/#\w+[^\n]*/g, (match) => {
            const placeholder = `__PREPROCESSOR_${placeholderIndex++}__`;
            placeholders.push(`<span class="preprocessor">${match}</span>`);
            return placeholder;
        });

        // Highlight keywords (word boundaries)
        keywords.forEach(keyword => {
            const regex = new RegExp(`\\b${keyword}\\b`, 'g');
            html = html.replace(regex, `<span class="keyword">${keyword}</span>`);
        });

        // Highlight types (word boundaries)
        types.forEach(type => {
            const regex = new RegExp(`\\b${type}\\b`, 'g');
            html = html.replace(regex, `<span class="type">${type}</span>`);
        });

        // Highlight functions
        functions.forEach(func => {
            const regex = new RegExp(`\\b${func}\\b`, 'g');
            html = html.replace(regex, `<span class="function">${func}</span>`);
        });

        // Highlight numbers
        html = html.replace(/\b\d+(\.\d+)?\b/g, '<span class="number">$&</span>');

        // Highlight operators (careful not to break HTML tags)
        const operators = ['=>', '->', '==', '!=', '<=', '>=', '&&', '||', '++', '--', '+=', '-=', '*=', '/='];
        operators.forEach(op => {
            const escaped = op.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
            html = html.replace(new RegExp(escaped, 'g'), `<span class="operator">${op}</span>`);
        });

        // Restore placeholders
        placeholders.forEach((replacement, index) => {
            const placeholder = index < placeholderIndex - placeholders.length + functions.length
                ? `__STRING_${index}__`
                : index < placeholderIndex - functions.length
                ? `__COMMENT_${index - (placeholderIndex - placeholders.length + functions.length)}__`
                : `__PREPROCESSOR_${index - (placeholderIndex - functions.length)}__`;

            // Simple replacement
            for (let i = 0; i < placeholders.length; i++) {
                html = html.replace(`__STRING_${i}__`, placeholders[i] || '');
                html = html.replace(`__COMMENT_${i}__`, placeholders[i] || '');
                html = html.replace(`__PREPROCESSOR_${i}__`, placeholders[i] || '');
            }
        });

        block.innerHTML = html;
    });
}

// Run highlighting when DOM is ready
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', highlightCbCode);
} else {
    highlightCbCode();
}