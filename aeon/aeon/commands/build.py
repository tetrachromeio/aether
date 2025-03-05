import os
import subprocess
import toml
from pathlib import Path
from aeon.utils.config_utils import read_global_config

def find_package_sources(package_dir):
    """Find all source files and includes for a package with aeon-specific structure"""
    sources = []
    includes = []
    
    package_root = Path(package_dir)
    
    # Find all C++ sources (including nested directories)
    for src_file in package_root.glob('**/Src/**/*.cpp'):
        sources.append(str(src_file))
    
    # Find aeon-specific include directories
    for include_dir in package_root.glob('**/Include/aeon'):
        if include_dir.is_dir():
            includes.append(f"-I{include_dir.parent}")  # Point to parent of aeon dir
    
    return sources, includes

def build_project():
    project_dir = os.getcwd()
    src_dir = os.path.join(project_dir, "src")
    build_dir = os.path.join(project_dir, "build")
    packages_dir = os.path.join(project_dir, "aeon-packages")
    
    os.makedirs(build_dir, exist_ok=True)

    # Read project configuration
    try:
        with open(os.path.join(project_dir, "aeon.toml")) as f:
            config = toml.load(f)
    except FileNotFoundError:
        print("Error: aeon.toml not found!")
        return

    # Read global aeon config
    global_config = read_global_config()
    if not global_config or "aeon_path" not in global_config:
        print("Error: aeon library not linked. Use 'aeon link <path_to_aeon>' first.")
        return

    aeon_path = global_config["aeon_path"]
    
    # Collect base aeon sources
    aeon_src_files = [
        str(Path(aeon_path) / "src/Core/EventLoop.cpp"),
        str(Path(aeon_path) / "src/Core/Print.cpp"),
        str(Path(aeon_path) / "src/Http/Server.cpp"),
        str(Path(aeon_path) / "src/Http/HttpParser.cpp"),
        str(Path(aeon_path) / "src/Http/Connection.cpp"),
        str(Path(aeon_path) / "src/Http/Middleware.cpp"),
        str(Path(aeon_path) / "src/Http/Response.cpp"),
        str(Path(aeon_path) / "src/Middleware/ServeStatic.cpp"),
    ]
    
    # Collect package sources with aeon-specific structure
    package_includes = []
    package_sources = []
    
    if "packages" in config.get("project", {}):
        for pkg_name, pkg_version in config["project"]["packages"].items():
            pkg_dir = Path(packages_dir) / pkg_name / pkg_version
            if not pkg_dir.exists():
                print(f"Error: Missing package {pkg_name}@{pkg_version}")
                print(f"Expected path: {pkg_dir}")
                return
                
            pkg_src, pkg_inc = find_package_sources(pkg_dir)
            if not pkg_inc:
                print(f"Error: Package {pkg_name} missing aeon include structure")
                print(f"Expected path: {pkg_dir}/Include/aeon/<package_files>")
                return
                
            package_sources.extend(pkg_src)
            package_includes.extend(pkg_inc)

    # Build command
    build_command = [
        "g++",
        "-std=c++17",
        f"-I{Path(aeon_path) / 'include'}",  # Core aeon includes
        *package_includes,  # Package includes
        "-o", str(Path(build_dir) / "main"),
        str(Path(src_dir) / "main.cpp"),  # Project source
        *aeon_src_files,  # aeon core sources
        *package_sources,  # Package sources
        "-lboost_system",
        "-pthread"
    ]

    # Verbose build info
    print("Building with command:")
    print(" ".join(build_command))
    print("\nInclude paths:")
    print("\n".join([f" - {i}" for i in build_command if i.startswith("-I")]))
    
    # Run build
    try:
        subprocess.run(build_command, check=True)
        print("\n✅ Build successful! Execute with:")
        print(f"   {Path(build_dir)/'main'}")
    except subprocess.CalledProcessError as e:
        print(f"\n❌ Build failed with error code {e.returncode}")
        print("Verify package structure:")
        print("1. Package includes should be in: aeon-packages/<name>/<version>/Include/aeon/...")
        print("2. Source files should be in: aeon-packages/<name>/<version>/Src/...")