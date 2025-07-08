import os
import shutil
import toml
from aeon.utils.file_utils import create_directory, write_file
import importlib.resources as pkg_resources
import aeon

def new_project(project_name, template="blank"):
    current_dir = os.getcwd()
    project_dir = os.path.join(current_dir, project_name)
    package_root = os.path.dirname(aeon.__file__)
    template_dir = os.path.join(package_root, "samples", template)

    if not os.path.isdir(template_dir):
        raise FileNotFoundError(f"Template '{template}' not found in aeon/samples/")

    # Copy template to new project directory
    shutil.copytree(template_dir, project_dir)

    # Load and modify aeon.toml
    aeon_toml_path = os.path.join(project_dir, "aeon.toml")
    if not os.path.isfile(aeon_toml_path):
        raise FileNotFoundError("aeon.toml missing in the template.")

    with open(aeon_toml_path, "r") as f:
        toml_data = toml.load(f)

    toml_data["project"]["name"] = project_name

    with open(aeon_toml_path, "w") as f:
        toml.dump(toml_data, f)

    print(f"Created new project: {project_name} using template: {template}")
