# Aeon Package Manager

**Modern Dependency Management for C++ Projects**  
![Version](https://img.shields.io/badge/version-0.1.0_Alpha-blue) 
![Status](https://img.shields.io/badge/status-Active_Development-brightgreen)

Aeon is a next-generation package manager designed specifically for C++ projects, combining familiar workflows from npm and cargo with C++'s unique requirements. Designed to work seamlessly with the Aether framework but usable with any C++ project.

```bash
# Create -> Link -> Build -> Run
aeon new my-project
cd my-project
aeon link /path/to/aether
aeon build
aeon run
```

## 🚀 Features

### Current Capabilities
- **Project scaffolding** with modern C++ structure
- **Dependency resolution** with semantic versioning
- **Cross-platform builds** using intelligent include detection
- **Package linking** for local development
- **Automatic rebuild** on source changes

### Coming Soon (v0.2.0)
- `aeon add <package>` - Add dependencies from registry
- Binary caching for frequent dependencies
- Header-only package support
- CI/CD integration templates

## 📦 Installation

```bash
# Clone and install
git clone https://github.com/tetrachromeio/aeon
cd aeon
pip install -e .
```

## 🛠 Getting Started

### Create a New Project
```bash
aeon new my-server
cd my-server
```

### Link Core Library
```bash
aeon link /path/to/aether-library
```

### Build & Run
```bash
aeon build  # Compiles project and dependencies
aeon run    # Starts the application
```

## 📂 Project Structure

Aeon creates standardized projects:
```
my-project/
├── aeon.toml       # Project configuration
├── Include/        # Local headers
├── src/            # Application source
│   └── main.cpp
└── aeon-packages/  # Downloaded dependencies
    └── websocket/
        └── 1.2.0/
            ├── Include/
            └── Src/
```

## 📝 aeon.toml Configuration

```toml
[project]
name = "my-app"
version = "0.1.0"
description = "High-performance web server"

[dependencies]
websockets = "1.2.0"  # Coming in v0.2.0
```

## 🔨 Build System

Aeon's smart builder:
- Automatically discovers nested dependencies
- Resolves include paths recursively
- Compiles with optimal flags for your platform
- Supports mixed source/header-only packages

## 📦 Package Management (Planned)

```bash
# Future workflow
aeon add middleware@1.0.0  # Add from registry
aeon add ./local-pkg       # Add local package
aeon install               # Install all dependencies
```

## 🤝 Contributing

We welcome contributions! Current priorities:
- `aeon add` command implementation
- Binary package distribution support
- Improved dependency resolution
- Windows build support

## 📜 License

MIT License - Free for open source and commercial use
