import os
import subprocess
import toml
import time
import json
import hashlib
import threading
import signal
import sys
from pathlib import Path
from datetime import datetime
from aeon.utils.config_utils import read_global_config
from aeon.utils.system_deps import SystemDependencyManager
from .build import get_file_hash, load_build_cache, save_build_cache, needs_rebuild, compile_source_file

class DevServer:
    def __init__(self):
        self.current_process = None
        self.should_stop = False
        self.is_building = False
        self.build_count = 0
        
    def generate_build_name(self):
        """Generate unique build name with timestamp"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        self.build_count += 1
        return f"dev_{timestamp}_{self.build_count}"
    
    def get_all_source_files(self):
        """Get all source files that should be watched (PROJECT FILES ONLY)"""
        project_dir = Path.cwd()
        src_dir = project_dir / "src"
        include_dir = project_dir / "include"
        packages_dir = project_dir / "aeon_modules" / "dependencies"
        
        # Read aeon.toml
        try:
            with open(project_dir / "aeon.toml") as f:
                config = toml.load(f)
        except FileNotFoundError:
            return [], []

        # Only watch PROJECT files, not Aether core files
        project_source_files = []
        watch_files = []
        
        # Main project source
        main_src = src_dir / "main.cpp"
        if main_src.exists():
            project_source_files.append(main_src)
            
        # Add all cpp files in project src directory
        for cpp_file in src_dir.rglob("*.cpp"):
            if cpp_file not in project_source_files:
                project_source_files.append(cpp_file)
        
        # Add project headers to watch list (not Aether headers)
        for header in src_dir.rglob("*.h"):
            watch_files.append(header)
        for header in include_dir.rglob("*.h"):
            watch_files.append(header)
            
        # Package sources (if any)
        if "dependencies" in config:
            for full_name, version in config["dependencies"].items():
                pkg_dir = packages_dir / full_name
                if pkg_dir.exists():
                    toml_path = pkg_dir / "package.toml"
                    if toml_path.exists():
                        pkg = toml.load(toml_path)
                        build = pkg.get("build", {})
                        sources = build.get("sources", [])
                        for src in sources:
                            project_source_files.append(pkg_dir / src)
        
        return project_source_files, watch_files
    
    def build_dev_version(self, build_name):
        """Build a development version with unique name"""
        print(f"🔨 Building {build_name}...")
        
        project_dir = Path.cwd()
        src_dir = project_dir / "src"
        build_dir = project_dir / "build" / "dev"
        obj_dir = build_dir / "obj" / build_name
        # Separate directory for Aether core objects (shared across builds)
        aether_obj_dir = build_dir / "aether_core_obj"
        include_dir = project_dir / "include"
        packages_dir = project_dir / "aeon_modules" / "dependencies"
        cache_file = build_dir / f"{build_name}_cache.json"
        aether_cache_file = build_dir / "aether_core_cache.json"
        
        build_dir.mkdir(parents=True, exist_ok=True)
        obj_dir.mkdir(parents=True, exist_ok=True)
        aether_obj_dir.mkdir(parents=True, exist_ok=True)
        
        # Load caches
        build_cache = load_build_cache(cache_file)
        aether_cache = load_build_cache(aether_cache_file)
        
        # Read aeon.toml
        try:
            with open(project_dir / "aeon.toml") as f:
                config = toml.load(f)
        except FileNotFoundError:
            print("❌ aeon.toml not found!")
            return None

        # Initialize system dependency manager
        system_dep_manager = SystemDependencyManager()
        system_dep_manager.set_cache_dir(build_dir)
        
        # Process system dependencies
        system_flags = {"include_flags": [], "lib_flags": [], "lib_dirs": []}
        if "systemDependencies" in config:
            print("🔧 Processing system dependencies...")
            system_flags = system_dep_manager.get_compilation_flags(config["systemDependencies"])

        # Read global config for aeon path
        global_config = read_global_config()
        if not global_config or "aeon_path" not in global_config:
            print("❌ Aeon not linked. Use 'aeon link <path>'")
            return None

        aeon_path = Path(global_config["aeon_path"])
        
        # Separate Aether core sources from project sources
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
        
        # Project sources (main.cpp and any other project files)
        project_source_files = []
        main_src = src_dir / "main.cpp"
        if main_src.exists():
            project_source_files.append(main_src)
        
        # Add all other cpp files in src directory
        for cpp_file in src_dir.rglob("*.cpp"):
            if cpp_file != main_src:
                project_source_files.append(cpp_file)
        
        # Initial include paths
        package_includes = [
            f"-I{aeon_path / 'include'}",
            f"-I{include_dir}"
        ]
        
        # Collect package sources and includes
        package_sources = []
        if "dependencies" in config:
            for full_name, version in config["dependencies"].items():
                pkg_dir = packages_dir / full_name
                if pkg_dir.exists():
                    toml_path = pkg_dir / "package.toml"
                    if toml_path.exists():
                        pkg = toml.load(toml_path)
                        build = pkg.get("build", {})
                        sources = build.get("sources", [])
                        includes = build.get("includes", [])
                        for src in sources:
                            package_sources.append(pkg_dir / src)
                        for inc in includes:
                            package_includes.append(f"-I{pkg_dir / inc}")
        
        # Compile Aether core files (only if needed)
        aether_object_files = []
        aether_files_compiled = 0
        
        print("🔍 Checking Aether core files...")
        for source_file in aeon_src_files:
            if not source_file.exists():
                continue
                
            rel_path = source_file.name.replace('.cpp', '.o')
            object_file = aether_obj_dir / rel_path
            
            # Only compile Aether files if they've changed
            if needs_rebuild(source_file, object_file, aether_cache, package_includes):
                print(f"🔨 Compiling Aether core: {source_file.name}")
                if not compile_source_file(source_file, object_file, package_includes, system_flags):
                    return None
                aether_files_compiled += 1
                
                # Update Aether cache
                aether_cache[str(source_file)] = {
                    'hash': get_file_hash(source_file),
                    'compiled_at': time.time()
                }
            
            aether_object_files.append(object_file)
        
        if aether_files_compiled > 0:
            print(f"✅ Compiled {aether_files_compiled} Aether core files")
        else:
            print("✅ Aether core files up to date")
        
        # Compile project files (these change more often)
        project_object_files = []
        project_files_compiled = 0
        
        print("🔍 Checking project files...")
        all_project_files = project_source_files + package_sources
        
        for source_file in all_project_files:
            if not source_file.exists():
                continue
                
            rel_path = source_file.name.replace('.cpp', '.o')
            object_file = obj_dir / rel_path
            
            # Always check project files for changes
            if needs_rebuild(source_file, object_file, build_cache, package_includes):
                if not compile_source_file(source_file, object_file, package_includes, system_flags):
                    return None
                project_files_compiled += 1
            
            project_object_files.append(object_file)
            
            # Update project cache
            build_cache[str(source_file)] = {
                'hash': get_file_hash(source_file),
                'compiled_at': time.time()
            }
        
        if project_files_compiled > 0:
            print(f"✅ Compiled {project_files_compiled} project files")
        else:
            print("✅ Project files up to date")
        
        # Link executable (combine Aether core + project objects)
        all_object_files = aether_object_files + project_object_files
        executable = build_dir / build_name
        link_command = [
            "g++",
            "-std=c++17",
            *[str(obj) for obj in all_object_files if obj.exists()],
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
            return None
        
        # Save caches
        save_build_cache(cache_file, build_cache)
        save_build_cache(aether_cache_file, aether_cache)
        
        print(f"✅ Built {build_name}")
        return executable
    
    def run_executable(self, executable):
        """Run the executable and return the process"""
        try:
            print(f"🚀 Starting {executable.name}...")
            process = subprocess.Popen([str(executable)], 
                                     stdout=subprocess.PIPE, 
                                     stderr=subprocess.STDOUT, 
                                     universal_newlines=True,
                                     bufsize=1)
            return process
        except Exception as e:
            print(f"❌ Failed to start {executable.name}: {e}")
            return None
    
    def stop_process(self):
        """Stop the current running process"""
        if self.current_process:
            try:
                self.current_process.terminate()
                self.current_process.wait(timeout=5)
                print("🛑 Stopped previous process")
            except subprocess.TimeoutExpired:
                self.current_process.kill()
                print("🛑 Force killed previous process")
            except Exception as e:
                print(f"⚠️  Error stopping process: {e}")
            finally:
                self.current_process = None
    
    def cleanup_old_builds(self, current_build_name):
        """Remove old build files but keep the current one and preserve Aether core objects"""
        build_dir = Path.cwd() / "build" / "dev"
        if not build_dir.exists():
            return
            
        for item in build_dir.iterdir():
            # Remove old executables
            if item.is_file() and item.name.startswith("dev_") and item.name != current_build_name:
                try:
                    item.unlink()
                    print(f"🗑️  Removed old build: {item.name}")
                except Exception as e:
                    print(f"⚠️  Could not remove {item.name}: {e}")
            # Clean old project object directories (but keep aether_core_obj)
            elif item.is_dir() and item.name == "obj":
                for obj_dir in item.iterdir():
                    if obj_dir.is_dir() and obj_dir.name.startswith("dev_") and obj_dir.name != current_build_name:
                        try:
                            import shutil
                            shutil.rmtree(obj_dir)
                            print(f"🗑️  Removed old objects: {obj_dir.name}")
                        except Exception as e:
                            print(f"⚠️  Could not remove {obj_dir.name}: {e}")
            # Keep aether_core_obj directory - it's shared across all builds!
    
    def has_file_changes(self, source_files, watch_files, last_check_time):
        """Check if any source or header files have changed"""
        all_files = source_files + watch_files
        for file_path in all_files:
            if file_path.exists() and file_path.stat().st_mtime > last_check_time:
                print(f"📝 Changed: {file_path.name}")
                return True
        return False
    
    def print_output(self, process):
        """Print process output in real-time"""
        while process.poll() is None and not self.should_stop:
            try:
                line = process.stdout.readline()
                if line:
                    print(line.rstrip())
            except:
                break
    
    def handle_user_input(self):
        """Handle user input for 'rs' command"""
        while not self.should_stop:
            try:
                user_input = input().strip().lower()
                if user_input == "rs":
                    print("🔄 Manual restart requested...")
                    self.manual_restart = True
            except (EOFError, KeyboardInterrupt):
                break
    
    def start_dev_server(self):
        """Start the development server with hot reload"""
        print("🔥 Starting Aeon Dev Server...")
        print("💡 Type 'rs' to manually restart")
        print("💡 Press Ctrl+C to stop")
        
        source_files, watch_files = self.get_all_source_files()
        if not source_files:
            print("❌ No source files found")
            return
        
        current_executable = None
        last_check_time = time.time()
        self.manual_restart = False
        
        # Initial build
        build_name = self.generate_build_name()
        executable = self.build_dev_version(build_name)
        
        if executable:
            current_executable = executable
            self.current_process = self.run_executable(executable)
            
            if self.current_process:
                # Start output thread
                output_thread = threading.Thread(target=self.print_output, args=(self.current_process,), daemon=True)
                output_thread.start()
        else:
            print("⚠️  Initial build failed, waiting for file changes to retry...")
            current_executable = None
        
        # Start input handler thread
        input_thread = threading.Thread(target=self.handle_user_input, daemon=True)
        input_thread.start()
        
        try:
            while not self.should_stop:
                time.sleep(1)
                
                current_time = time.time()
                
                # Check for file changes or manual restart
                has_changes = self.has_file_changes(source_files, watch_files, last_check_time)
                
                if (has_changes or self.manual_restart):
                    
                    if self.manual_restart:
                        print("🔄 Manual restart...")
                        self.manual_restart = False
                    else:
                        print("📝 File changes detected...")
                    
                    if not self.is_building:
                        self.is_building = True
                        
                        # Build new version (smart incremental build)
                        new_build_name = self.generate_build_name()
                        new_executable = self.build_dev_version(new_build_name)
                        
                        if new_executable:
                            # Stop old process
                            self.stop_process()
                            
                            # Start new process
                            self.current_process = self.run_executable(new_executable)
                            
                            # Clean up old builds
                            if current_executable and current_executable != new_executable:
                                self.cleanup_old_builds(new_build_name)
                            
                            current_executable = new_executable
                            
                            # Start new output thread
                            if self.current_process:
                                output_thread = threading.Thread(target=self.print_output, args=(self.current_process,), daemon=True)
                                output_thread.start()
                        else:
                            # Build failed, but don't break - just wait for next file change
                            print("⚠️  Build failed, waiting for file changes to retry...")
                        
                        self.is_building = False
                    
                    # Only update last_check_time after we've processed the changes
                    last_check_time = current_time
                
                # Check if process died
                if self.current_process and self.current_process.poll() is not None:
                    print("💀 Process exited")
                    break
                    
        except KeyboardInterrupt:
            print("\n🛑 Stopping dev server...")
        finally:
            self.should_stop = True
            self.stop_process()


def dev_server():
    """Start the development server"""
    server = DevServer()
    
    # Handle Ctrl+C gracefully
    def signal_handler(sig, frame):
        server.should_stop = True
        server.stop_process()
        print("\n👋 Dev server stopped")
        sys.exit(0)
    
    signal.signal(signal.SIGINT, signal_handler)
    
    try:
        server.start_dev_server()
    except Exception as e:
        print(f"❌ Dev server error: {e}")
        server.stop_process()
