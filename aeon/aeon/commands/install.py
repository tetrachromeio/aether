import os
import shutil
import subprocess
from pathlib import Path
from tomlkit import parse, dumps
from tomlkit.toml_file import TOMLFile


def install(pkg_list: list[str]):
    packages = parse_packages(pkg_list)

    aeon_toml_path = Path("aeon.toml")
    if not aeon_toml_path.exists():
        print("❌ aeon.toml not found in the current directory.")
        return

    aeon = TOMLFile(aeon_toml_path).read()

    for pkg in packages:
        pkg_id = pkg['id']
        version = pkg['version']
        print(f"🔍 Searching for package {pkg_id} version {version}...")

        user, repo = pkg_id.split("/")
        repo_url = f"https://github.com/{user}/{repo}.git"
        cache_dir = Path("aeon_modules/cache") / user / repo
        final_dir = Path("aeon_modules/dependencies") / user / repo

        # Clean existing cache dir
        if cache_dir.exists():
            shutil.rmtree(cache_dir)

        print(f"⬇️ Cloning {repo_url} into cache...")
        try:
            subprocess.run(["git", "clone", "--depth", "1", repo_url, str(cache_dir)], check=True)
        except subprocess.CalledProcessError:
            print(f"❌ Failed to clone {repo_url}")
            continue

        package_toml_path = cache_dir / "package.toml"
        if not package_toml_path.exists():
            print(f"❌ package.toml not found in {pkg_id}, skipping.")
            shutil.rmtree(cache_dir)
            continue

        try:
            pkg_toml = TOMLFile(package_toml_path).read()
        except Exception as e:
            print(f"❌ Invalid TOML in {pkg_id}: {e}")
            shutil.rmtree(cache_dir)
            continue

        # Passed validation — update aeon.toml
        aeon.setdefault("dependencies", {})
        aeon["dependencies"][pkg_id] = version

        aeon.setdefault("locked", {})
        aeon["locked"][pkg_id] = version

        # Move to final install path
        print(f"📦 Moving {pkg_id} to dependencies folder...")
        final_dir.parent.mkdir(parents=True, exist_ok=True)
        if final_dir.exists():
            shutil.rmtree(final_dir)
        shutil.move(str(cache_dir), str(final_dir))

    # Save aeon.toml changes
    print("📝 Writing updates to aeon.toml...")
    TOMLFile(aeon_toml_path).write(aeon)

    print("✅ Installation complete.")


def parse_packages(pkg_list: list[str]):
    parsed = []
    for pkg in pkg_list:
        if '@' in pkg:
            pkg_id, version = pkg.split('@', 1)
        else:
            pkg_id, version = pkg, 'latest'
        parsed.append({'id': pkg_id, 'version': version})
    return parsed
