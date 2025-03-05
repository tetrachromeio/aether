import os
import subprocess
from aeon.commands.build import build_project

def run_project():
    project_dir = os.getcwd()
    build_dir = os.path.join(project_dir, "build")
    executable = os.path.join(build_dir, "main")

    # Build the project if the executable doesn't exist
    if not os.path.exists(executable):
        print("Executable not found. Building project...")
        build_project()

    # Run the executable
    try:
        subprocess.run([executable], check=True)
    except subprocess.CalledProcessError as e:
        print(f"Failed to run project: {e}")