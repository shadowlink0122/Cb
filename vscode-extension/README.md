# Cb Language Support for Visual Studio Code

This extension provides syntax highlighting and language support for the Cb programming language.

## Features

- **Syntax Highlighting**: Full syntax highlighting support for Cb language constructs
- **Auto-completion**: Bracket, quote, and parenthesis auto-closing
- **Code Folding**: Support for region-based code folding
- **Comment Support**: Line (`//`) and block (`/* */`) comment toggling

## Supported Language Features

### Keywords and Control Flow
- Control structures: `if`, `else`, `for`, `while`, `break`, `continue`, `return`
- Pattern matching: `match`, `case`, `switch`, `default`
- Advanced control: `defer`, `yield`, `async`, `await`
- Error handling: `try`, `checked`, `panic`, `unwrap`

### Data Types
- Primitive types: `tiny`, `short`, `int`, `long`, `float`, `double`, `char`, `string`, `bool`, `void`
- Type definitions: `struct`, `enum`, `interface`, `typedef`, `union`
- Type modifiers: `const`, `static`, `private`, `unsigned`

### Advanced Features
- Generics support
- Async/await syntax
- String interpolation with `{variable}`
- Pointers and references
- Function pointers

## Installation

### From VSIX
1. Download the `.vsix` file from the releases page
2. Open VS Code
3. Go to Extensions view (Ctrl+Shift+X / Cmd+Shift+X)
4. Click the "..." menu and select "Install from VSIX..."
5. Select the downloaded `.vsix` file

### Manual Installation
1. Copy the extension folder to your VS Code extensions directory:
   - **Windows**: `%USERPROFILE%\.vscode\extensions`
   - **macOS/Linux**: `~/.vscode/extensions`
2. Restart VS Code

## Usage

Once installed, the extension will automatically activate when you open a `.cb` file.

## Example Code

```cb
// Cb language example
import "std/io";

struct Point {
    int x;
    int y;
};

int distance(Point p1, Point p2) {
    int dx = p2.x - p1.x;
    int dy = p2.y - p1.y;
    return dx * dx + dy * dy;
}

async int compute(int n) {
    return n * 2;
}

void main() {
    Point p1 = {x: 0, y: 0};
    Point p2 = {x: 3, y: 4};

    int dist = distance(p1, p2);
    println("Distance squared: {dist}");

    int result = await compute(42);
    println("Result: {result}");
}
```

## Language Version

This extension supports Cb language **v0.13.0**.

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests to the [GitHub repository](https://github.com/shadowlink0122/Cb).

## License

MIT

## Release Notes

### 0.13.0 (Initial Release)
- Initial release with full syntax highlighting support
- Support for all Cb language keywords and constructs
- Auto-closing pairs for brackets, quotes, and parentheses
- Code folding support
- Comment toggling support
