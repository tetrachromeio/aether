"""
Test suite for Aeon CLI commands
"""

import unittest
import tempfile
import shutil
import os
from pathlib import Path
from unittest.mock import patch, MagicMock
import sys

# Add the aeon package to the Python path
sys.path.insert(0, str(Path(__file__).parent.parent / "aeon"))

from aeon.commands.new import new_project
from aeon.commands.build import build_project
from aeon.commands.link import link_aeon


class TestAeonCommands(unittest.TestCase):
    """Test core Aeon CLI commands"""
    
    def setUp(self):
        """Set up test environment"""
        self.test_dir = tempfile.mkdtemp()
        self.original_cwd = os.getcwd()
        os.chdir(self.test_dir)
    
    def tearDown(self):
        """Clean up test environment"""
        os.chdir(self.original_cwd)
        shutil.rmtree(self.test_dir)
    
    def test_new_project_blank_template(self):
        """Test creating a new project with blank template"""
        project_name = "test_project"
        
        # Create the project
        new_project(project_name, "blank")
        
        # Verify project structure
        project_path = Path(self.test_dir) / project_name
        self.assertTrue(project_path.exists())
        self.assertTrue((project_path / "aeon.toml").exists())
        self.assertTrue((project_path / "src").exists())
        self.assertTrue((project_path / "src" / "main.cpp").exists())
        
        # Verify aeon.toml content
        with open(project_path / "aeon.toml") as f:
            content = f.read()
            self.assertIn(f'name = "{project_name}"', content)
    
    def test_new_project_aether_template(self):
        """Test creating a new project with aether template"""
        project_name = "test_aether_project"
        
        # Create the project
        new_project(project_name, "aether")
        
        # Verify project structure
        project_path = Path(self.test_dir) / project_name
        self.assertTrue(project_path.exists())
        self.assertTrue((project_path / "aeon.toml").exists())
        self.assertTrue((project_path / "src").exists())
        self.assertTrue((project_path / "src" / "main.cpp").exists())
        
        # Verify aeon.toml content includes aether-specific config
        with open(project_path / "aeon.toml") as f:
            content = f.read()
            self.assertIn("systemDependencies", content)
            self.assertIn("openssl", content)
    
    @patch('aeon.commands.build.read_global_config')
    def test_build_without_linked_aeon(self, mock_config):
        """Test build command fails when aeon is not linked"""
        mock_config.return_value = None
        
        # Create a minimal project structure
        project_name = "test_build"
        new_project(project_name, "blank")
        os.chdir(Path(self.test_dir) / project_name)
        
        # Build should fail without linked aeon
        with patch('builtins.print') as mock_print:
            build_project()
            mock_print.assert_any_call("❌ Aeon not linked. Use 'aeon link <path>'")
    
    @patch('aeon.utils.config_utils.write_global_config')
    def test_link_aeon(self, mock_write_config):
        """Test linking aeon library"""
        fake_path = "/fake/aeon/path"
        
        # Mock path existence
        with patch('os.path.exists', return_value=True):
            link_aeon(fake_path)
            
            # Verify config was written
            mock_write_config.assert_called_once()
            args = mock_write_config.call_args[0][0]
            self.assertEqual(args["aeon_path"], fake_path)
    
    def test_link_aeon_invalid_path(self):
        """Test linking with invalid path fails"""
        fake_path = "/nonexistent/path"
        
        with patch('builtins.print') as mock_print:
            link_aeon(fake_path)
            mock_print.assert_any_call(f"❌ Path does not exist: {fake_path}")


class TestProjectStructure(unittest.TestCase):
    """Test project structure validation"""
    
    def setUp(self):
        """Set up test environment"""
        self.test_dir = tempfile.mkdtemp()
        self.original_cwd = os.getcwd()
        os.chdir(self.test_dir)
    
    def tearDown(self):
        """Clean up test environment"""
        os.chdir(self.original_cwd)
        shutil.rmtree(self.test_dir)
    
    def test_blank_template_structure(self):
        """Test that blank template creates correct structure"""
        project_name = "blank_test"
        new_project(project_name, "blank")
        
        project_path = Path(self.test_dir) / project_name
        
        # Check all required files and directories exist
        required_items = [
            "aeon.toml",
            "src",
            "src/main.cpp",
            "Include"
        ]
        
        for item in required_items:
            self.assertTrue((project_path / item).exists(), f"Missing {item}")
    
    def test_aether_template_structure(self):
        """Test that aether template creates correct structure"""
        project_name = "aether_test"
        new_project(project_name, "aether")
        
        project_path = Path(self.test_dir) / project_name
        
        # Check all required files and directories exist
        required_items = [
            "aeon.toml",
            "src",
            "src/main.cpp",
            "src/models",
            "src/models/admin.toml",
            "src/models/user.toml",
            "public"
        ]
        
        for item in required_items:
            self.assertTrue((project_path / item).exists(), f"Missing {item}")


if __name__ == '__main__':
    unittest.main()
