import os
import subprocess
import toml
import time
import json
import hashlib
from pathlib import Path
from aeon.utils.config_utils import read_global_config
from aeon.utils.system_deps import SystemDependencyManager

def get_file_hash(file_path):
        # Link if needed
    if needs_linking:
        print("🔗 Linking executable...")
        link_command = [
            "g++",
            "-std=c++17",
            *[str(obj) for obj in object_files if obj.exists()],
            "-o", str(executable),
        ]
        
        # Add system dependency library directories and flags
        if system_flags["lib_dirs"]:
            link_command.extend(system_flags["lib_dirs"])
        if system_flags["lib_flags"]:
            link_command.extend(system_flags["lib_flags"])
        
        # Add default libraries
        link_command.extend([
            "-lboost_system",
            "-pthread"
        ])
        
        result = subprocess.run(link_command, capture_output=True, text=True)
        if result.returncode != 0:
            print(f"❌ Linking failed")
            print(f"Error: {result.stderr}")
            return
    else:
        print("✨ Executable up to date!")
    if not file_path.exists():
        return None
    with open(file_path, 'rb') as f:
        return hashlib.md5(f.read()).hexdigest()

def load_build_cache(cache_file):
    """Load build cache from JSON file"""
    if not cache_file.exists():
        return {}
    try:
        with open(cache_file) as f:
            return json.load(f)
    except (json.JSONDecodeError, FileNotFoundError):
        return {}

def save_build_cache(cache_file, cache_data):
    """Save build cache to JSON file"""
    with open(cache_file, 'w') as f:
        json.dump(cache_data, f, indent=2)

def needs_rebuild(source_file, object_file, build_cache, include_dirs):
    """Check if a source file needs to be rebuilt"""
    if not object_file.exists():
        return True
    
    source_str = str(source_file)
    
    # Check if source file changed
    current_hash = get_file_hash(source_file)
    if source_str not in build_cache or build_cache[source_str].get('hash') != current_hash:
        return True
    
    # Check if any header dependencies changed
    source_mtime = source_file.stat().st_mtime
    object_mtime = object_file.stat().st_mtime
    
    if source_mtime > object_mtime:
        return True
    
    # Check include directories for newer files
    for include_dir in include_dirs:
        include_path = Path(include_dir.replace('-I', ''))
        if include_path.exists():
            for header in include_path.rglob('*.h'):
                if header.stat().st_mtime > object_mtime:
                    return True
    
    return False

def compile_source_file(source_file, object_file, include_dirs, system_flags=None):
    """Compile a single source file to object file"""
    object_file.parent.mkdir(parents=True, exist_ok=True)
    
    compile_command = [
        "g++",
        "-std=c++17",
        "-c",  # Compile only, don't link
        *include_dirs,
    ]
    
    # Add system dependency include flags
    if system_flags and "include_flags" in system_flags:
        compile_command.extend(system_flags["include_flags"])
    
    compile_command.extend([
        "-o", str(object_file),
        str(source_file)
    ])
    
    print(f"🔨 Compiling {source_file.name}...")
    result = subprocess.run(compile_command, capture_output=True, text=True)
    
    if result.returncode != 0:
        print(f"❌ Failed to compile {source_file.name}")
        print(f"Error: {result.stderr}")
        return False
    
    return True

def build_project():
    start_time = time.time()
    
    project_dir = Path.cwd()
    src_dir = project_dir / "src"
    build_dir = project_dir / "build"
    obj_dir = build_dir / "obj"
    include_dir = project_dir / "include"
    packages_dir = project_dir / "aeon_modules" / "dependencies"
    cache_file = build_dir / "build_cache.json"
    
    print("⚙️  Preparing build environment...")
    build_dir.mkdir(parents=True, exist_ok=True)
    obj_dir.mkdir(parents=True, exist_ok=True)

    # Load build cache
    build_cache = load_build_cache(cache_file)
    
    # Initialize system dependency manager
    system_dep_manager = SystemDependencyManager()
    system_dep_manager.set_cache_dir(build_dir)
    
    # Read aeon.toml
    try:
        print("📄 Reading aeon.toml...")
        with open(project_dir / "aeon.toml") as f:
            config = toml.load(f)
    except FileNotFoundError:
        print("❌ aeon.toml not found!")
        return

    # Process system dependencies
    system_flags = {"include_flags": [], "lib_flags": [], "lib_dirs": []}
    if "systemDependencies" in config:
        print("🔧 Processing system dependencies...")
        system_flags = system_dep_manager.get_compilation_flags(config["systemDependencies"])
        if system_flags["include_flags"]:
            print(f"📁 System includes: {' '.join(system_flags['include_flags'])}")
        if system_flags["lib_flags"] or system_flags["lib_dirs"]:
            lib_info = ' '.join(system_flags["lib_dirs"] + system_flags["lib_flags"])
            print(f"🔗 System libraries: {lib_info}")
    
    # Read global config for aeon path
    print("🌍 Reading global aeon configuration...")
    global_config = read_global_config()
    if not global_config or "aeon_path" not in global_config:
        print("❌ Aeon not linked. Use 'aeon link <path>'")
        return

    aeon_path = Path(global_config["aeon_path"])

    # Collect all source files
    print("🔍 Collecting source files...")
    all_source_files = []
    object_files = []
    
    # Aeon core sources
    aeon_src_files = [
        aeon_path / "src/Core/EventLoop.cpp",
        aeon_path / "src/Core/Print.cpp",
        aeon_path / "src/Http/Server.cpp",
        aeon_path / "src/Http/HttpParser.cpp",
        aeon_path / "src/Http/Connection.cpp",
        aeon_path / "src/Http/Middleware.cpp",
        aeon_path / "src/Http/Response.cpp",
        aeon_path / "src/Http/RoutePattern.cpp",
        aeon_path / "src/Http/Router.cpp",
        aeon_path / "src/Middleware/ServeStatic.cpp",
    ]
    
    # Initial include paths: Aeon core + main project
    package_includes = [
        f"-I{aeon_path / 'include'}",
        f"-I{include_dir}"
    ]
    
    # Collect package sources and includes
    def collect_package_sources(pkg_name, pkg_version):
        pkg_dir = packages_dir / pkg_name
        if not pkg_dir.exists():
            print(f"❌ Missing package {pkg_name}@{pkg_version}")
            return []

        toml_path = pkg_dir / "package.toml"
        if not toml_path.exists():
            print(f"❌ package.toml missing in {pkg_name}")
            return []

        pkg = toml.load(toml_path)
        build = pkg.get("build", {})
        sources = build.get("sources", [])
        includes = build.get("includes", [])

        pkg_sources = []
        for src in sources:
            pkg_sources.append(pkg_dir / src)
        for inc in includes:
            package_includes.append(f"-I{pkg_dir / inc}")
        
        return pkg_sources

    package_sources = []
    if "dependencies" in config:
        print("📦 Processing dependencies...")
        for full_name, version in config["dependencies"].items():
            package_sources.extend(collect_package_sources(full_name, version))

    # Add main project source
    main_src = src_dir / "main.cpp"
    if not main_src.exists():
        print(f"❌ {main_src} not found.")
        return

    # Collect all source files
    all_source_files = [main_src] + aeon_src_files + package_sources
    
    # Incremental compilation
    print("🔍 Checking what needs to be rebuilt...")
    files_to_compile = []
    
    for source_file in all_source_files:
        if not source_file.exists():
            print(f"❌ Source file not found: {source_file}")
            continue
            
        # Generate object file path
        rel_path = source_file.name.replace('.cpp', '.o')
        object_file = obj_dir / rel_path
        
        if needs_rebuild(source_file, object_file, build_cache, package_includes):
            files_to_compile.append((source_file, object_file))
        
        object_files.append(object_file)
    
    # Compile changed files
    if files_to_compile:
        print(f"🔨 Compiling {len(files_to_compile)} changed files...")
        for source_file, object_file in files_to_compile:
            if not compile_source_file(source_file, object_file, package_includes, system_flags):
                return
            
            # Update cache
            build_cache[str(source_file)] = {
                'hash': get_file_hash(source_file),
                'compiled_at': time.time()
            }
    else:
        print("✨ All source files up to date!")
    
    # Check if linking is needed
    executable = build_dir / "main"
    needs_linking = False
    
    if not executable.exists():
        needs_linking = True
    else:
        executable_mtime = executable.stat().st_mtime
        for obj_file in object_files:
            if obj_file.exists() and obj_file.stat().st_mtime > executable_mtime:
                needs_linking = True
                break
    
    # Link if needed
    if needs_linking:
        print("🔗 Linking executable...")
        link_command = [
            "g++",
            "-std=c++17",
            *[str(obj) for obj in object_files if obj.exists()],
            "-o", str(executable),
        ]
        
        # Add system dependency library directories and flags
        if system_flags["lib_dirs"]:
            link_command.extend(system_flags["lib_dirs"])
        if system_flags["lib_flags"]:
            link_command.extend(system_flags["lib_flags"])
        
        # Add default libraries
        link_command.extend([
            "-lboost_system",
            "-pthread"
        ])
        
        result = subprocess.run(link_command, capture_output=True, text=True)
        if result.returncode != 0:
            print(f"❌ Linking failed")
            print(f"Error: {result.stderr}")
            return
    else:
        print("✨ Executable up to date!")
    
    # Save cache
    save_build_cache(cache_file, build_cache)
    
    compile_time = time.time() - start_time
    
    if files_to_compile or needs_linking:
        print(f"✅ Build successful in {compile_time:.2f}s")
    else:
        print(f"✅ Build up to date ({compile_time:.2f}s)")
    print("💡 Run it with: aeon run")

def clean_build():
    """Clean build artifacts and cache"""
    project_dir = Path.cwd()
    build_dir = project_dir / "build"
    
    if not build_dir.exists():
        print("✨ Already clean - no build directory found")
        return
    
    # Remove object files
    obj_dir = build_dir / "obj"
    if obj_dir.exists():
        import shutil
        shutil.rmtree(obj_dir)
        print("🗑️  Removed object files")
    
    # Remove system dependency cache
    system_deps_dir = build_dir / "system_deps"
    if system_deps_dir.exists():
        import shutil
        shutil.rmtree(system_deps_dir)
        print("🗑️  Removed system dependency cache")
    
    # Remove cache
    cache_file = build_dir / "build_cache.json"
    if cache_file.exists():
        cache_file.unlink()
        print("🗑️  Removed build cache")
    
    # Remove executable
    executable = build_dir / "main"
    if executable.exists():
        executable.unlink()
        print("🗑️  Removed executable")
    
    print("✅ Clean complete")
