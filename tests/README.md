# Alif LSP Tests

Simple unit tests for the Alif Language Server Protocol implementation.

## Running Tests

### Option 1: Visual Studio (Recommended for Windows)
1. Open `tests/alif_tests.vcxproj` in Visual Studio
2. Build and run the project (Ctrl+F5)

### Option 2: Command Line

#### Windows (with Visual Studio)
```bash
cd tests
build_and_run_tests_vs.bat
```

#### Windows (with MinGW/g++)
```bash
cd tests
build_and_run_tests.bat
```

#### Linux/macOS
```bash
cd tests
./build_and_run_tests.sh
```

## Test Coverage

### `getCurrentWord()` Function Tests
- ✅ Empty document handling
- ✅ Cursor at beginning of line
- ✅ Cursor in middle of word
- ✅ Cursor at end of word
- ✅ Cursor after whitespace
- ✅ Partial word extraction
- ✅ Multiline document handling

### `getSuggestions()` Function Tests
- ✅ Empty document returns all keywords
- ✅ Partial input filters suggestions
- ✅ Exact match scenarios
- ✅ No match returns empty results
- ✅ Suggestion item structure validation

## Test Framework

Uses a simple custom test framework (`test_framework.h`) with:
- `assert_equal()` for string comparisons
- `assert_true()` for boolean checks
- `run_test()` for test execution
- Automatic test summary and reporting

## Adding New Tests

1. Create test functions in `test_completion.cpp`
2. Add them to the `main()` function using `TestFramework::run_test()`
3. Use the assertion functions to validate expected behavior

## Dependencies

- C++17 compiler (g++ or MSVC)
- Alif LSP source files
- nlohmann/json (included in third-party)
