# Comprehensive Fix Checklist

## Completed ✓
1. ✓ Updated profile.jpg to square crop from IMG_2154.jpg
2. ✓ Changed all "CB" to "Cb" (case-sensitive)
3. ✓ Updated date to 2025/11/21
4. ✓ Updated line count to 74,000
5. ✓ Fixed Claude 3.5 Sonnet → Claude Sonnet 4.5
6. ✓ Fixed development period duplication (2025年7月〜)
7. ✓ Removed "質問タイム" slide, replaced with ending slide
8. ✓ Updated community section about not welcoming direct commits

## Remaining Issues to Fix

### Code Syntax Fixes
- [ ] Constructor syntax: Show it's in `impl Resource` block
- [ ] Remove slice arrays from Cb syntax comparison (not supported)
- [ ] Fix function pointer syntax
- [ ] Fix lambda syntax
- [ ] Fix import statements (remove quotes, use stdlib.std.vector format)
- [ ] Fix Vector/Queue usage (use .at, .push, .pop - no .init)
- [ ] Fix async/await (remove explicit Future wrapping)
- [ ] Add interface/impl comparison (interface - Go, impl - Rust)

### Syntax Highlighting
- [ ] Make async/await/match/for/while same color (#c586c0)
- [ ] Make interface/impl red (#c586c0) like other keywords
- [ ] Fix comment italics (make normal, not italic)
- [ ] Highlight method calls (.push_back, .at, .pop, owner, balance) with #dcdcaa

### Content Organization
- [ ] Add "Cbとは" slide before "なぜCbを作ったのか"
- [ ] Add constructor/destructor/defer slide before pattern matching
- [ ] Add encapsulation (private/default) slide
- [ ] Add import statement example slide
- [ ] Remove duplicate Result+async slide (combine them)

### Specific Code Examples
- [ ] Fix constructor example to show `impl Resource { self(int id) { ... } }`
- [ ] Fix Vector example to match stdlib/std/vector.cb actual API
- [ ] Fix Queue example to match stdlib/std/queue.cb actual API
- [ ] Fix async example - remove Future wrapping
- [ ] Add interface/impl comparison table

### Page Organization
- [ ] Ensure all pages have consistent title placement
- [ ] Ensure 20-minute presentation flow
- [ ] Verify all code fits on one line where requested
