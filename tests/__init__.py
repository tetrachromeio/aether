"""
Aeon Package Manager Test Suite
Test configuration and setup
"""

import sys
import os
from pathlib import Path

# Add the aeon package to the Python path for testing
sys.path.insert(0, str(Path(__file__).parent.parent / "aeon"))
