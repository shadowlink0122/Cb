# Change Log

All notable changes to the "cb-language" extension will be documented in this file.

## [0.13.2] - 2025-11-13

### Changed
- `static`, `const`, `unsigned` keywords are now highlighted in blue (like C++)
- Primitive types (`int`, `long`, `float`, `double`, etc.) are now highlighted in blue
- Constants (all uppercase with underscores, e.g., `MAX_VALUE`) are now highlighted as constants
- Number literals are consistently highlighted as constants
- Improved variable pattern to exclude uppercase constants

## [0.13.1] - 2025-11-13

### Added
- Preprocessor directive syntax highlighting (#define, #undef, #ifdef, #ifndef, #if, #elif, #elseif, #else, #endif, #error, #warning, #include)
- Built-in macro highlighting (__FILE__, __LINE__, __DATE__, __TIME__, __VERSION__)
- Preprocessor directives are now highlighted in pink (like C/C++)

### Changed
- Preprocessor patterns are now processed before comments for proper highlighting

## [0.13.0] - 2025-11-12

### Added
- Initial release of Cb Language Support extension
- Full syntax highlighting for Cb language v0.13.0
- Support for all language keywords:
  - Control flow: `if`, `else`, `for`, `while`, `break`, `continue`, `return`
  - Pattern matching: `match`, `case`, `switch`, `default`
  - Advanced control: `defer`, `yield`, `async`, `await`
  - Error handling: `try`, `checked`, `panic`, `unwrap`
- Support for primitive types: `tiny`, `short`, `int`, `long`, `float`, `double`, `char`, `string`, `bool`, `void`
- Support for type definitions: `struct`, `enum`, `interface`, `typedef`, `union`
- Support for type modifiers: `const`, `static`, `private`, `unsigned`
- Syntax highlighting for:
  - Comments (line and block)
  - Strings with escape sequences
  - String interpolation with `{variable}`
  - Numeric literals (decimal, hex, binary, octal, float)
  - Operators (arithmetic, logical, bitwise, comparison, assignment)
  - Function declarations and calls
  - Constants (`true`, `false`, `nullptr`, `null`)
- Auto-closing pairs for brackets, quotes, and parentheses
- Smart indentation rules
- Code folding support with region markers
- Comment toggling support

### Language Features
- Full support for Cb language specification v0.13.0
- Generics syntax highlighting
- Async/await syntax support
- Pointer and reference operators
- Function pointer type definitions
- Union types and pattern matching

## Format

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).
