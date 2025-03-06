# aeon/commands/__init__.py

# Import all commands for easier access
from .new import new_project
from .build import build_project
from .run import run_project
from .link import link_aeon


# Optional: Define a list of available commands
__all__ = ["new_project", "build_project", "run_project", "link_aeon", "login_user", "register_user"]