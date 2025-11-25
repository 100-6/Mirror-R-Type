# Contributing to R-Type

Thank you for your interest in contributing to R-Type! This document provides guidelines and instructions for contributing to the project.

## Table of Contents

- [Getting Started](#getting-started)
- [Development Environment Setup](#development-environment-setup)
- [Code Quality Tools](#code-quality-tools)
- [Building the Project](#building-the-project)
- [Code Style](#code-style)
- [Submitting Changes](#submitting-changes)

## Getting Started

1. Fork the repository
2. Clone your fork: `git clone https://github.com/YOUR_USERNAME/Mirror-R-Type.git`
3. Create a feature branch: `git checkout -b feature/your-feature-name`
4. Make your changes
5. Test your changes
6. Submit a pull request

## Development Environment Setup

### Prerequisites

- **C++ Compiler**: GCC 10+ or Clang 12+ with C++20 support
- **CMake**: Version 3.20 or higher
- **Git**: For version control

### Required Development Tools

#### clang-tidy (Code Linter)

clang-tidy is used for static code analysis and enforcing code quality standards.

**Installation:**

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install clang-tidy
```

**Fedora/RedHat:**
```bash
sudo dnf install clang-tools-extra
```

**macOS:**
```bash
brew install llvm
```

**Windows:**
```bash
# Using Chocolatey
choco install llvm

# Or download from https://releases.llvm.org/
```

**Verify Installation:**
```bash
clang-tidy --version
```

## Code Quality Tools

### Running clang-tidy

The project includes clang-tidy integration for code quality checks.

#### During Build (Optional)

To enable clang-tidy analysis during compilation:

```bash
cmake -DENABLE_CLANG_TIDY=ON -B build
cmake --build build
```

**Note:** This is disabled by default to speed up regular builds.

#### Manual Analysis

To analyze specific files:

```bash
# Single file
clang-tidy path/to/file.cpp -- -std=c++20

# All C++ files
find . -name '*.cpp' -o -name '*.hpp' | xargs clang-tidy -p build

# With compile commands database
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B build
clang-tidy -p build path/to/file.cpp
```

#### Auto-fix Issues

To automatically apply suggested fixes:

```bash
clang-tidy -fix path/to/file.cpp -- -std=c++20
```

### Configuration

The project's clang-tidy configuration is located in [.clang-tidy](.clang-tidy) at the repository root.

For detailed linting documentation, see [doc/LINTING.md](doc/LINTING.md).

## Building the Project

### Standard Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Debug Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Build Options

- `BUILD_CLIENT`: Build client executable (default: ON)
- `BUILD_SERVER`: Build server executable (default: ON)
- `ENABLE_CLANG_TIDY`: Enable clang-tidy during build (default: OFF)

Example:
```bash
cmake -S . -B build -DBUILD_CLIENT=OFF -DBUILD_SERVER=ON
```

## Code Style

### Naming Conventions

- **Namespaces**: `lower_case`
- **Classes/Structs**: `CamelCase`
- **Functions**: `camelBack`
- **Variables**: `camelBack`
- **Private/Protected Members**: suffix with `_`
- **Macros**: `UPPER_CASE`
- **Enum Constants**: `UPPER_CASE`

### Example

```cpp
namespace game_engine {

class EntityManager {
public:
    void addEntity(Entity* entity);
    size_t getEntityCount() const;

private:
    std::vector<Entity*> entities_;
    size_t maxEntities_;
};

} // namespace game_engine
```

### Best Practices

- Use C++20 features when appropriate
- Prefer smart pointers over raw pointers
- Follow RAII principles
- Write self-documenting code
- Add comments only when necessary to explain "why", not "what"
- Keep functions small and focused
- Avoid magic numbers; use named constants

## Submitting Changes

### Before Submitting

1. **Build the project** to ensure it compiles without errors
2. **Run clang-tidy** on your changes to catch potential issues
3. **Test your changes** thoroughly
4. **Write clear commit messages** following the format:
   ```
   type: brief description

   Detailed explanation of what changed and why.
   ```

   Types: `feat`, `fix`, `refactor`, `docs`, `test`, `chore`

### Pull Request Process

1. **Update documentation** if you've changed APIs or added features
2. **Ensure CI passes**: All GitHub Actions workflows must succeed
3. **Target the correct branch**:
   - Features and fixes: target `dev` branch
   - Hotfixes: may target `main` branch (discuss with maintainers first)
4. **Request review** from maintainers
5. **Address feedback** promptly and professionally

### CI/CD

The project uses GitHub Actions for continuous integration:

- **Build Job**: Builds the project on Ubuntu and Windows
- **Lint Job**: Runs clang-tidy analysis (non-blocking, only on PRs to `main`)

Pull requests to `main` will include clang-tidy analysis results as artifacts. While the linter is non-blocking, please address warnings when reasonable.

## Questions?

If you have questions or need help, feel free to:

- Open an issue for bugs or feature requests
- Start a discussion for general questions
- Reach out to the maintainers

Thank you for contributing to R-Type!
