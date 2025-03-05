import os
import json

def get_global_config_path():
    """Get the path to the global configuration file."""
    home_dir = os.path.expanduser("~")
    config_dir = os.path.join(home_dir, ".aeon")
    config_file = os.path.join(config_dir, "config.json")
    return config_file

def read_global_config():
    """Read the global configuration file."""
    config_file = get_global_config_path()
    if not os.path.exists(config_file):
        return None
    with open(config_file, "r") as f:
        return json.load(f)

def write_global_config(config):
    """Write to the global configuration file."""
    config_file = get_global_config_path()
    config_dir = os.path.dirname(config_file)
    os.makedirs(config_dir, exist_ok=True)
    with open(config_file, "w") as f:
        json.dump(config, f, indent=4)