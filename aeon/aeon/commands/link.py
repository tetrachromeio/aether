import os
import json

from aeon.utils.config_utils import write_global_config

def link_aeon(aeon_path):
    # Ensure the path exists
    if not os.path.exists(aeon_path):
        print(f"Error: Path '{aeon_path}' does not exist.")
        return

    # Update the global configuration
    config = {
        "aeon_path": aeon_path
    }
    write_global_config(config)

    print(f"Linked aeon library at: {aeon_path}")