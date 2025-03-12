# CLAUDE.md - PlatformIO Storage Test Project Guidelines

## Build Commands
- Build and upload: `platformio run --target upload`
- Upload filesystem image: `platformio run --target uploadfs`
- Monitor serial output: `platformio device monitor`
- Clean build: `platformio run --target clean`

## Test Commands
- Run all tests: `platformio test`
- Run specific test: `platformio test -e <environment> -f <test_file>`

## Code Style Guidelines
- **File Structure**: Headers (.h) in include/, implementations (.cpp) in src/
- **Naming**: snake_case for functions, PascalCase for classes, camelCase for variables
- **Constants**: UPPER_SNAKE_CASE for constants and macros
- **Includes**: Standard libraries first, then project headers
- **Error Handling**: Functions return boolean success/failure values
- **Documentation**: Comments describe function purpose, parameters and return values
- **Performance**: Use yield() to prevent watchdog timeouts, implement buffering
- **Safety**: Bounds checking, input validation, retry critical operations