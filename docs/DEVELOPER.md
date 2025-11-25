# Developer Guide

Welcome to the Mirror R-Type development team! This guide will help you set up your development environment and understand our workflow.

## Table of Contents

- [Development Environment](#development-environment)
- [Code Style](#code-style)
- [Development Workflow](#development-workflow)
- [Testing](#testing)
- [Documentation](#documentation)
- [Common Tasks](#common-tasks)

---

## Development Environment

### Required Tools

In addition to the [Setup Guide](SETUP.md) requirements, developers should install:

#### Linux/macOS

```bash
# clang-tidy (C++ linter)
sudo apt-get install clang-tidy  # Debian/Ubuntu
brew install llvm                 # macOS

# clang-format (C++ formatter)
sudo apt-get install clang-format # Debian/Ubuntu
brew install clang-format         # macOS

# Optional: gdb or lldb for debugging
sudo apt-get install gdb          # Linux
brew install lldb                 # macOS
```

#### Windows

Install via Visual Studio Installer:
- Clang tools
- C++ AddressSanitizer
- C++ profiling tools

---

### IDE Setup

#### Visual Studio Code

Recommended extensions:

```json
{
  "recommendations": [
    "ms-vscode.cpptools",
    "ms-vscode.cmake-tools",
    "llvm-vs-code-extensions.vscode-clangd",
    "xaver.clang-format",
    "notskm.clang-tidy"
  ]
}
```

#### CLion

CLion has native CMake support. Simply open the project directory.

---

## Code Style

### C++ Standards

- **C++20** features encouraged
- Use modern C++ idioms (RAII, smart pointers, STL containers)
- Avoid raw pointers and manual memory management
- Prefer `constexpr` and `const` where possible

### Naming Conventions

```cpp
// Classes: PascalCase
class NetworkPlugin {};

// Functions/Methods: snake_case
void send_packet();

// Variables: snake_case
int player_count = 0;

// Constants: UPPER_SNAKE_CASE
constexpr int MAX_PLAYERS = 4;

// Private members: trailing underscore
class Player {
private:
    int health_;
    Vector2f position_;
};
```

### File Structure

```cpp
// Header file (.hpp)
#pragma once

namespace engine {

class ClassName {
public:
    // Public interface

private:
    // Private implementation
};

} // namespace engine
```

```cpp
// Implementation file (.cpp)
#include "ClassName.hpp"

namespace engine {

// Implementation

} // namespace engine
```

---

### Code Formatting

We use **clang-format** with the project's `.clang-format` configuration.

#### Format your code:

```bash
# Format a single file
clang-format -i src/MyFile.cpp

# Format all files
find engine client server -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
```

#### Auto-format in VS Code:

Set in `.vscode/settings.json`:

```json
{
  "editor.formatOnSave": true,
  "C_Cpp.clang_format_style": "file"
}
```

---

### Linting with clang-tidy

#### Enable during build:

```bash
cmake -B build -DENABLE_CLANG_TIDY=ON
cmake --build build
```

#### Run manually:

```bash
clang-tidy engine/src/*.cpp -- -I engine/include
```

#### Configuration

See `.clang-tidy` in the project root for our ruleset.

---

## Development Workflow

### Branching Strategy

We follow **Git Flow**:

- `main` - Production-ready code
- `develop` - Integration branch for features
- `feature/*` - New features
- `bugfix/*` - Bug fixes
- `hotfix/*` - Urgent production fixes

### Creating a Feature

```bash
# Create feature branch
git checkout develop
git pull origin develop
git checkout -b feature/my-awesome-feature

# Make changes and commit
git add .
git commit -m "feat: add awesome feature"

# Push and create PR
git push origin feature/my-awesome-feature
```

### Commit Message Format

We follow [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <description>

[optional body]

[optional footer]
```

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting)
- `refactor`: Code refactoring
- `test`: Adding or updating tests
- `chore`: Build process or auxiliary tool changes

**Examples:**

```bash
git commit -m "feat(network): implement UDP packet fragmentation"
git commit -m "fix(graphics): resolve memory leak in texture loading"
git commit -m "docs: update plugin creation guide"
git commit -m "refactor(ecs): simplify entity component lookup"
```

---

### Pull Request Process

1. **Create PR** from your feature branch to `develop`
2. **Fill out PR template** with:
   - Description of changes
   - Related issue numbers
   - Testing performed
3. **Request review** from at least one team member
4. **Address feedback** and update PR
5. **Squash and merge** once approved

---

## Testing

### Running Tests

```bash
# Build tests
cmake -B build -DBUILD_TESTS=ON
cmake --build build

# Run all tests
ctest --test-dir build --output-on-failure

# Run specific test
./build/tests/plugin_manager_test
./build/tests/network_test
```

### Writing Tests

We use **Google Test**:

```cpp
#include <gtest/gtest.h>
#include "MyClass.hpp"

TEST(MyClassTest, DoSomething) {
    MyClass obj;
    EXPECT_EQ(obj.getValue(), 42);
    ASSERT_TRUE(obj.isValid());
}
```

Place tests in `tests/` directory with `_test.cpp` suffix.

---

### Code Coverage

```bash
# Build with coverage flags
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
cmake --build build

# Run tests
ctest --test-dir build

# Generate coverage report
lcov --capture --directory build --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
```

Open `coverage_report/index.html` in a browser.

---

## Documentation

### Inline Documentation

Use Doxygen-style comments:

```cpp
/**
 * @brief Sends a network packet to the specified client
 *
 * @param packet The packet to send
 * @param client_id Target client identifier
 * @return true if packet was sent successfully, false otherwise
 *
 * @throws NetworkException if connection is lost
 */
bool send_to(const NetworkPacket& packet, ClientId client_id);
```

### Documentation Files

- Architecture docs in `docs/`
- API docs inline in headers
- README files in relevant directories

### Generating API Docs

```bash
doxygen Doxyfile
# Output in docs/api/html/
```

---

## Common Tasks

### Adding a New Plugin

See [Plugin Creation Guide](../engine/plugin_manager/HOW_TO_CREATE_PLUGIN.md)

### Adding a New System

1. Create header in `engine/include/systems/`
2. Implement in `engine/src/systems/`
3. Register in engine initialization
4. Add tests in `tests/systems/`

### Adding a New Component

```cpp
// engine/include/components/MyComponent.hpp
#pragma once

namespace engine {

struct MyComponent {
    int value;
    float multiplier;
};

} // namespace engine
```

Register with ECS:

```cpp
registry.register_component<MyComponent>();
```

---

### Debugging

#### GDB (Linux)

```bash
gdb ./build/r_type_client
(gdb) run
(gdb) backtrace  # On crash
```

#### LLDB (macOS)

```bash
lldb ./build/r_type_client
(lldb) run
(lldb) bt  # On crash
```

#### Visual Studio (Windows)

Press F5 to debug with breakpoints.

---

### Performance Profiling

#### Linux (perf)

```bash
perf record ./build/r_type_server
perf report
```

#### macOS (Instruments)

```bash
instruments -t "Time Profiler" ./build/r_type_server
```

#### Windows (Visual Studio Profiler)

Debug → Performance Profiler → CPU Usage

---

## Best Practices

### Do's ✅

- Write unit tests for new features
- Document public APIs
- Use const-correctness
- Handle errors gracefully
- Keep functions small and focused
- Use meaningful variable names
- Follow the existing code style

### Don'ts ❌

- Don't commit commented-out code
- Don't hardcode magic numbers
- Don't use `goto`
- Don't ignore compiler warnings
- Don't leak memory
- Don't expose implementation details in headers
- Don't commit directly to `main` or `develop`

---

## Code Review Checklist

- [ ] Code follows style guide
- [ ] Tests pass locally
- [ ] New tests added for new features
- [ ] Documentation updated
- [ ] No compiler warnings
- [ ] clang-tidy passes
- [ ] Commit messages follow convention
- [ ] PR description is clear

---

## Resources

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [Modern CMake](https://cliutils.gitlab.io/modern-cmake/)
- [Google Test Documentation](https://google.github.io/googletest/)
- [Boost.Asio Documentation](https://www.boost.org/doc/libs/release/doc/html/boost_asio.html)

---

## Getting Help

- Ask in team chat
- Check [GitHub Issues](https://github.com/your-repo/issues)
- Read [existing documentation](../docs/)
- Pair program with a team member

---

**Happy Coding!** 🚀
