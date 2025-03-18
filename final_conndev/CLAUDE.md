# CLAUDE.md - Connected Device Platform Guidelines

## Build Commands
- Build and upload: `pio run --target upload`
- Upload filesystem image: `pio run --target uploadfs`
- Monitor serial output: `pio device monitor`
- Clean build: `pio run --target clean`
- Process web files: `uv run src/data_prep.py`

## Test Commands
- Run all tests: `pio test`
- Run specific test: `pio test -e nanorp2040connect -f <test_file>`

## Code Style Guidelines
- **File Structure**: Headers (.h) in include/, implementations (.cpp) in src/
- **Naming**: snake_case for functions, PascalCase for classes, camelCase for variables
- **Constants**: UPPER_SNAKE_CASE for constants and macros
- **Includes**: Standard libraries first, then project headers
- **Error Handling**: Functions return boolean success/failure values
- **Documentation**: Comments describe function purpose, parameters and return values
- **Performance**: Use yield() to prevent watchdog timeouts, implement buffering
- **Safety**: Bounds checking, input validation, retry critical operations
- **WiFi**: Always provide fallback to AP mode if client mode fails
- **Storage**: Check storage before writing, implement flush checks to prevent data loss