# Contributing to Aether

Thank you for your interest in contributing to Aether! Whether you're fixing bugs, improving documentation, or proposing new features, your contributions are welcome and appreciated. This guide will help you get started.

---

## 🛠️ How to Contribute

### 1. **Report Issues**
If you find a bug or have a feature request, please [open an issue](https://github.com/tetrachromeio/aether/issues). Be sure to:
- Use a clear and descriptive title.
- Include steps to reproduce the issue (if applicable).
- Specify your environment (OS, compiler, etc.).

### 2. **Submit Pull Requests**
If you'd like to contribute code:
1. **Fork the repository** and clone it locally.
2. Create a new branch for your changes:
   ```bash
   git checkout -b my-feature-branch
   ```
3. Make your changes and test them thoroughly.
4. Commit your changes with a clear and concise message:
   ```bash
   git commit -m "feat: add support for WebSocket"
   ```
5. Push your branch to your fork:
   ```bash
   git push origin my-feature-branch
   ```
6. Open a **Pull Request (PR)** against the `main` branch of the Aether repository.

---

## 🌟 Contribution Guidelines

### Code Style
- Follow the existing code style (e.g., indentation, naming conventions).
- Use descriptive variable and function names.
- Include comments where necessary to explain complex logic.

### Commit Messages
- Use the [Conventional Commits](https://www.conventionalcommits.org/) format:
  - `feat:` for new features.
  - `fix:` for bug fixes.
  - `docs:` for documentation changes.
  - `chore:` for maintenance tasks.
  - `refactor:` for code refactoring.
  - `test:` for adding or updating tests.

### Testing
- Ensure your changes are thoroughly tested.
- Add unit tests for new features or bug fixes.
- Run the existing test suite to verify no regressions.

### Documentation
- Update the README, CONTRIBUTING, or other docs if your changes affect them.
- Add comments to new code to explain its purpose and usage.

---

## 🚀 Getting Started

### Prerequisites
- **C++17+ compiler** (GCC, Clang, or MSVC).
- **Boost.Asio** (for networking).
- **Python 3.6+** (for `cpm` CLI).

### Setup
1. **Fork and clone the repository**:
   ```bash
   git clone https://github.com/tetrachromeio/aether.git
   cd Aether
   ```
2. **Install `cpm`**:
   ```bash
   cd cpm
   pip install .
   ```
3. **Build and test**:
   ```bash
   cpm build
   cpm run
   ```

---

## 🧑‍💻 Development Workflow

1. **Create a new branch** for your changes:
   ```bash
   git checkout -b my-feature-branch
   ```
2. **Make your changes** and test them locally.
3. **Run tests** (if applicable):
   ```bash
   # Add your test commands here
   ```
4. **Commit your changes**:
   ```bash
   git commit -m "feat: add support for WebSocket"
   ```
5. **Push your branch**:
   ```bash
   git push origin my-feature-branch
   ```
6. **Open a Pull Request** on GitHub.

---

## 📜 Code of Conduct

We are committed to fostering a welcoming and inclusive community. Please read and adhere to our [Code of Conduct](CODE_OF_CONDUCT.md).

---

## ❓ Need Help?

If you have questions or need assistance:
- Open a [GitHub Discussion](https://github.com/tetrachromeio/aether/discussions).

---

## 🙏 Thank You!

Your contributions help make Aether better for everyone. We appreciate your time and effort!

---

**Happy Coding!** 🚀
