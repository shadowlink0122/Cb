# Unified Test Framework for Cb Language

## Overview

The unified test framework allows running the same `.cb` test files in both **interpreter mode** and **compiler mode** to ensure consistency between both execution methods.

## Purpose

- **Consistency Verification**: Ensure that Cb code produces the same results whether interpreted or compiled
- **Regression Prevention**: Catch discrepancies between interpreter and compiler implementations early
- **Future Extensibility**: Designed to support MIR and LIR compilation stages when implemented

## Usage

### Run Tests in Both Modes
```bash
make integration-test-unified
```
This runs all `.cb` test files in `tests/cases/` directory in both interpreter and compiler modes.

### Run Tests in Specific Mode
```bash
# Interpreter mode only
make integration-test-unified-interpreter

# Compiler mode only  
make integration-test-unified-compiler
```

### Direct Script Usage
```bash
# Both modes (default)
bash tests/run_unified_integration_tests.sh both

# Interpreter only
bash tests/run_unified_integration_tests.sh interpreter

# Compiler only
bash tests/run_unified_integration_tests.sh compiler
```

## Output Format

The test runner provides clear visual feedback:

```
✓ INTERP | ✓ COMP [OUTPUT MATCH] basic/simple_main.cb
✓ INTERP | ✗ COMP [OUTPUT DIFF] array/basic.cb
✗ INTERP | ✓ COMP assign/int/ng.cb
```

- `✓ INTERP`: Test passed in interpreter mode
- `✗ INTERP`: Test failed in interpreter mode
- `✓ COMP`: Test passed in compiler mode
- `✗ COMP`: Test failed in compiler mode
- `[OUTPUT MATCH]`: Both modes produced identical output
- `[OUTPUT DIFF]`: Both modes passed but produced different output

## Test Statistics

At the end of each run, the framework provides a summary:

```
=============================================================
  Test Summary
=============================================================
Interpreter Mode:
  Total:  847 tests
  Passed: 820
  Failed: 27

Compiler Mode:
  Total:  847 tests
  Passed: 450
  Failed: 397

Total Tests: 847
```

## Implementation Details

### Execution Methods

**Interpreter Mode:**
```bash
./cb run <test_file.cb>
```

**Compiler Mode:**
```bash
./cb compile <test_file.cb> -o <output_binary>
<output_binary>
```

### Test Discovery

The framework automatically discovers all `.cb` files in the `tests/cases/` directory and its subdirectories.

### Output Comparison

When both modes pass, the framework compares their outputs to detect inconsistencies:
- Identical outputs are marked with `[OUTPUT MATCH]`
- Different outputs are marked with `[OUTPUT DIFF]`

## Differences from Legacy Integration Tests

### Legacy (C++ Framework)
- Tests written in C++ using integration test framework
- Requires recompilation when adding tests
- Located in `tests/integration/`
- Run with: `make integration-test`

### Unified (Cb Test Files)
- Tests written in Cb language
- No recompilation needed for new tests
- Uses actual `.cb` test files from `tests/cases/`
- Run with: `make integration-test-unified`

## Future Extensions

The framework is designed to support additional compilation stages:

```bash
# Planned for future
./cb compile-mir <test_file.cb> -o <output>
./cb compile-lir <test_file.cb> -o <output>
```

The same test cases can be used across all execution modes to ensure consistency throughout the compilation pipeline.

## Recent Fixes

### v0.14.0 - println Output Consistency

Fixed issue where compiled code added extra spaces in `println` output:

**Before:**
```
Hello World 
Test Multiple Args 
```

**After:**
```
Hello World
Test Multiple Args
```

The fix ensures that `println` behaves identically in both interpreter and compiler modes by using a proper fold expression that doesn't add trailing spaces.

## Contributing

When adding new test cases:

1. Add `.cb` files to appropriate subdirectory in `tests/cases/`
2. Test files are automatically discovered by the unified test framework
3. Tests should work in both interpreter and compiler modes when possible
4. Document any known differences between modes in the test file comments

## Related Files

- `tests/run_unified_integration_tests.sh` - Main test runner script
- `Makefile` - Integration with build system
- `tests/cases/` - Test case directory
