import subprocess
import os
import platform
from pathlib import Path

class SystemDependencyManager:
    def __init__(self):
        self.system = platform.system().lower()
        self.cache_dir = None
        
    def set_cache_dir(self, cache_dir):
        """Set the directory where system dependencies should be cached"""
        self.cache_dir = Path(cache_dir) / "system_deps"
        self.cache_dir.mkdir(parents=True, exist_ok=True)
    
    def detect_package_config(self, package_name):
        """Use pkg-config to detect system package configuration"""
        try:
            # Try pkg-config first
            result = subprocess.run(
                ["pkg-config", "--exists", package_name],
                capture_output=True,
                text=True
            )
            if result.returncode == 0:
                # Get cflags and libs
                cflags_result = subprocess.run(
                    ["pkg-config", "--cflags", package_name],
                    capture_output=True,
                    text=True
                )
                libs_result = subprocess.run(
                    ["pkg-config", "--libs", package_name],
                    capture_output=True,
                    text=True
                )
                
                if cflags_result.returncode == 0 and libs_result.returncode == 0:
                    return {
                        "cflags": cflags_result.stdout.strip().split(),
                        "libs": libs_result.stdout.strip().split(),
                        "method": "pkg-config"
                    }
        except FileNotFoundError:
            pass
        
        return None
    
    def detect_homebrew_package(self, package_name):
        """Detect packages installed via Homebrew on macOS"""
        if self.system != "darwin":
            return None
            
        try:
            # Check if homebrew is available
            result = subprocess.run(["brew", "--version"], capture_output=True, text=True)
            if result.returncode != 0:
                return None
                
            # Get homebrew prefix
            prefix_result = subprocess.run(
                ["brew", "--prefix", package_name],
                capture_output=True,
                text=True
            )
            
            if prefix_result.returncode == 0:
                prefix = prefix_result.stdout.strip()
                include_dir = f"{prefix}/include"
                lib_dir = f"{prefix}/lib"
                
                if os.path.exists(include_dir) and os.path.exists(lib_dir):
                    return {
                        "include_dir": include_dir,
                        "lib_dir": lib_dir,
                        "method": "homebrew"
                    }
        except FileNotFoundError:
            pass
        
        return None
    
    def detect_system_paths(self, package_name):
        """Check common system paths for libraries"""
        common_include_paths = [
            "/usr/include",
            "/usr/local/include",
            "/opt/include",
            f"/usr/include/{package_name}",
            f"/usr/local/include/{package_name}"
        ]
        
        common_lib_paths = [
            "/usr/lib",
            "/usr/local/lib",
            "/opt/lib",
            "/usr/lib/x86_64-linux-gnu",  # Ubuntu/Debian
            "/usr/lib64"  # CentOS/RHEL
        ]
        
        found_includes = []
        found_libs = []
        
        for path in common_include_paths:
            if os.path.exists(path):
                found_includes.append(path)
        
        for path in common_lib_paths:
            if os.path.exists(path):
                found_libs.append(path)
        
        if found_includes or found_libs:
            return {
                "include_dirs": found_includes,
                "lib_dirs": found_libs,
                "method": "system_paths"
            }
        
        return None
    
    def resolve_system_dependency(self, dep_name, dep_config):
        """Resolve a system dependency configuration"""
        # Check cache first
        cache_file = None
        if self.cache_dir:
            cache_file = self.cache_dir / f"{dep_name}.json"
            if cache_file.exists():
                import json
                try:
                    with open(cache_file) as f:
                        cached = json.load(f)
                        print(f"📦 Using cached system dependency: {dep_name}")
                        return cached
                except:
                    pass
        
        print(f"🔍 Resolving system dependency: {dep_name}")
        
        resolved = {
            "name": dep_name,
            "include_flags": [],
            "lib_flags": [],
            "lib_dirs": [],
            "resolved": False
        }
        
        # If it's a simple list format, convert to dict
        if isinstance(dep_config, list):
            dep_config = {"libs": dep_config}
        
        package_name = dep_config.get("package", dep_name)
        
        # Try different detection methods
        detection_result = None
        
        # 1. Try pkg-config
        detection_result = self.detect_package_config(package_name)
        if detection_result:
            resolved["include_flags"] = [flag for flag in detection_result["cflags"] if flag.startswith("-I")]
            resolved["lib_flags"] = detection_result["libs"]
            resolved["resolved"] = True
            resolved["method"] = "pkg-config"
        
        # 2. Try homebrew (macOS)
        elif self.system == "darwin":
            detection_result = self.detect_homebrew_package(package_name)
            if detection_result:
                resolved["include_flags"] = [f"-I{detection_result['include_dir']}"]
                resolved["lib_dirs"] = [f"-L{detection_result['lib_dir']}"]
                resolved["method"] = "homebrew"
                resolved["resolved"] = True
        
        # 3. Use explicit configuration from toml
        if not resolved["resolved"] and dep_config:
            if "includes" in dep_config:
                resolved["include_flags"] = [f"-I{inc}" for inc in dep_config["includes"]]
            
            if "lib_dirs" in dep_config:
                resolved["lib_dirs"] = [f"-L{lib_dir}" for lib_dir in dep_config["lib_dirs"]]
            
            resolved["resolved"] = True
            resolved["method"] = "explicit"
        
        # Add library linking flags
        if "libs" in dep_config:
            resolved["lib_flags"].extend([f"-l{lib}" for lib in dep_config["libs"]])
        
        # Cache the result
        if cache_file and resolved["resolved"]:
            import json
            cache_file.parent.mkdir(parents=True, exist_ok=True)
            with open(cache_file, 'w') as f:
                json.dump(resolved, f, indent=2)
        
        if resolved["resolved"]:
            print(f"✅ Resolved {dep_name} via {resolved.get('method', 'unknown')}")
        else:
            print(f"⚠️  Could not resolve system dependency: {dep_name}")
        
        return resolved
    
    def get_compilation_flags(self, system_deps):
        """Get compilation and linking flags for all system dependencies"""
        include_flags = []
        lib_flags = []
        lib_dirs = []
        
        for dep_name, dep_config in system_deps.items():
            resolved = self.resolve_system_dependency(dep_name, dep_config)
            if resolved["resolved"]:
                include_flags.extend(resolved["include_flags"])
                lib_flags.extend(resolved["lib_flags"])
                lib_dirs.extend(resolved["lib_dirs"])
        
        return {
            "include_flags": include_flags,
            "lib_flags": lib_flags,
            "lib_dirs": lib_dirs
        }
