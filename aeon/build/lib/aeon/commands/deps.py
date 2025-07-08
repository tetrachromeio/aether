import toml
from pathlib import Path
from aeon.utils.system_deps import SystemDependencyManager

def show_system_dependencies():
    """Show system dependencies configuration and status"""
    project_dir = Path.cwd()
    
    try:
        print("📄 Reading aeon.toml...")
        with open(project_dir / "aeon.toml") as f:
            config = toml.load(f)
    except FileNotFoundError:
        print("❌ aeon.toml not found!")
        return
    
    if "systemDependencies" not in config:
        print("📦 No system dependencies configured")
        return
    
    print("\n🔧 System Dependencies Configuration:")
    print("=" * 50)
    
    # Initialize system dependency manager
    build_dir = project_dir / "build"
    system_dep_manager = SystemDependencyManager()
    system_dep_manager.set_cache_dir(build_dir)
    
    for dep_name, dep_config in config["systemDependencies"].items():
        print(f"\n📦 {dep_name}:")
        
        if isinstance(dep_config, list):
            print(f"   Libraries: {', '.join(dep_config)}")
        else:
            if "package" in dep_config:
                print(f"   Package: {dep_config['package']}")
            if "libs" in dep_config:
                print(f"   Libraries: {', '.join(dep_config['libs'])}")
            if "includes" in dep_config:
                print(f"   Include dirs: {', '.join(dep_config['includes'])}")
        
        # Try to resolve the dependency
        resolved = system_dep_manager.resolve_system_dependency(dep_name, dep_config)
        
        if resolved["resolved"]:
            print(f"   ✅ Resolved via {resolved.get('method', 'unknown')}")
            if resolved["include_flags"]:
                print(f"   Include flags: {' '.join(resolved['include_flags'])}")
            if resolved["lib_dirs"]:
                print(f"   Library dirs: {' '.join(resolved['lib_dirs'])}")
            if resolved["lib_flags"]:
                print(f"   Link flags: {' '.join(resolved['lib_flags'])}")
        else:
            print(f"   ❌ Could not resolve")
    
    print("\n💡 Tip: Use 'aeon build' to compile with these dependencies")

def check_system_dependencies():
    """Check if all system dependencies can be resolved"""
    project_dir = Path.cwd()
    
    try:
        with open(project_dir / "aeon.toml") as f:
            config = toml.load(f)
    except FileNotFoundError:
        print("❌ aeon.toml not found!")
        return False
    
    if "systemDependencies" not in config:
        print("✅ No system dependencies to check")
        return True
    
    print("🔍 Checking system dependencies...")
    
    # Initialize system dependency manager
    build_dir = project_dir / "build"
    system_dep_manager = SystemDependencyManager()
    system_dep_manager.set_cache_dir(build_dir)
    
    all_resolved = True
    
    for dep_name, dep_config in config["systemDependencies"].items():
        resolved = system_dep_manager.resolve_system_dependency(dep_name, dep_config)
        if not resolved["resolved"]:
            all_resolved = False
    
    if all_resolved:
        print("✅ All system dependencies resolved successfully")
    else:
        print("❌ Some system dependencies could not be resolved")
        print("💡 Try installing missing packages with your system package manager")
        print("   - macOS: brew install <package>")
        print("   - Ubuntu/Debian: sudo apt install lib<package>-dev")
        print("   - CentOS/RHEL: sudo yum install <package>-devel")
    
    return all_resolved
