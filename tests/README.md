# Aeon Test Suite

This directory contains the comprehensive test suite for the Aeon package manager.

## Test Structure

- `test_commands.py` - Tests for CLI commands (new, build, link, etc.)
- `test_utils.py` - Tests for utility functions (config, file operations, TOML handling)
- `test_integration.py` - Integration tests for complete workflows
- `run_tests.py` - Test runner with coverage support

## Running Tests

### Basic Test Execution
```bash
# Run all tests
python run_tests.py

# Run with coverage
python run_tests.py --coverage

# Run specific test pattern
python run_tests.py --pattern "test_commands.py"
```

### Using unittest directly
```bash
# Run specific test file
python -m unittest test_commands.py

# Run specific test class
python -m unittest test_commands.TestAeonCommands

# Run specific test method
python -m unittest test_commands.TestAeonCommands.test_new_project_blank_template
```

## Test Coverage Goals

The test suite aims for:
- **80%+ overall code coverage**
- **100% coverage of critical paths** (project creation, building, linking)
- **Integration testing** of complete workflows
- **Error handling validation**

## Test Categories

### Unit Tests
- Individual function testing
- Mocked external dependencies
- Fast execution

### Integration Tests  
- Complete workflow testing
- File system operations
- Cross-component interactions

### Functional Tests
- End-to-end command testing
- Real file operations
- Template validation

## Adding New Tests

When adding new functionality:

1. **Add unit tests** for individual functions
2. **Add integration tests** for workflows
3. **Update this README** with new test descriptions
4. **Ensure coverage** meets minimum thresholds

## Dependencies

Core testing dependencies (included in Python standard library):
- `unittest` - Test framework
- `tempfile` - Temporary file/directory creation
- `pathlib` - Path operations
- `unittest.mock` - Mocking support

Optional dependencies:
- `coverage` - Coverage reporting (`pip install coverage`)

## CI/CD Integration

Tests are designed to run in CI/CD environments:
- No external dependencies required for basic tests
- Isolated test environments using temporary directories
- Clear pass/fail indicators
- Coverage reporting support
