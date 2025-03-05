import os
import subprocess
import toml
import time
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
    start_time = time.time()  # Start timing the build process
    
    project_dir = os.getcwd()
    src_dir = os.path.join(project_dir, "src")
    build_dir = os.path.join(project_dir, "build")
    packages_dir = os.path.join(project_dir, "aeon_modules")
    
    print("⚙️  Preparing build environment...")
    os.makedirs(build_dir, exist_ok=True)

    # Read project configuration
    try:
        print("📄 Reading aeon.toml...")
        with open(os.path.join(project_dir, "aeon.toml")) as f:
            config = toml.load(f)
    except FileNotFoundError:
        print("❌ Error: aeon.toml not found!")
        return

    # Read global aeon config
    print("🌍 Reading global aeon configuration...")
    global_config = read_global_config()
    if not global_config or "aeon_path" not in global_config:
        print("❌ Error: aeon library not linked. Use 'aeon link <path_to_aeon>' first.")
        return

    aeon_path = global_config["aeon_path"]
    
    # Collect base aeon sources
    print("🔍 Collecting aeon core sources...")
    aeon_src_files = [
        str(Path(aeon_path) / "src/Core/EventLoop.cpp"),
        str(Path(aeon_path) / "src/Core/Print.cpp"),
        str(Path(aeon_path) / "src/Http/Server.cpp"),
        str(Path(aeon_path) / "src/Http/HttpParser.cpp"),
        str(Path(aeon_path) / "src/Http/Connection.cpp"),
        str(Path(aeon_path) / "src/Http/Middleware.cpp"),
        str(Path(aeon_path) / "src/Http/Response.cpp"),
        str(Path(aeon_path) / "src/Http/RoutePattern.cpp"),
        str(Path(aeon_path) / "src/Middleware/ServeStatic.cpp"),
    ]
    
    # Collect package sources and includes
    package_includes = []
    package_sources = []
    
    if "dependencies" in config:
        print("📦 Processing dependencies...")
        for pkg_name, pkg_version in config["dependencies"].items():
            pkg_dir = Path(packages_dir) / pkg_name / pkg_version
            if not pkg_dir.exists():
                print(f"❌ Error: Missing package {pkg_name}@{pkg_version}")
                print(f"Expected path: {pkg_dir}")
                return
                
            # Read package.toml
            try:
                with open(pkg_dir / "package.toml") as f:
                    pkg_config = toml.load(f)
            except FileNotFoundError:
                print(f"❌ Error: package.toml not found for {pkg_name}@{pkg_version}")
                return

            # Add sources and includes
            if "build" in pkg_config:
                for src in pkg_config["build"].get("sources", []):
                    package_sources.append(str(pkg_dir / src))
                for inc in pkg_config["build"].get("includes", []):
                    package_includes.append(f"-I{str(pkg_dir / inc)}")

    # Build command
    print("🔨 Compiling project...")
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

    # Run build
    try:
        print("🚀 Starting build process...")
        subprocess.run(build_command, check=True)
        build_time = time.time() - start_time
        print(f"\n✅ Build successful! (took {build_time:.2f} seconds)")
        print("💡 Execute with:")
        print("aeon run")
    except subprocess.CalledProcessError as e:
        build_time = time.time() - start_time
        print(f"\n❌ Build failed with error code {e.returncode} (took {build_time:.2f} seconds)")
        print("💡 Check the error messages above for more details.")

if __name__ == "__main__":
    build_project()