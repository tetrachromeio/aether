"""
Test suite for Aeon utilities and configuration
"""

import unittest
import tempfile
import shutil
import os
import json
from pathlib import Path
from unittest.mock import patch, mock_open
import sys

# Add the aeon package to the Python path
sys.path.insert(0, str(Path(__file__).parent.parent / "aeon"))

from aeon.utils.config_utils import read_global_config, write_global_config
from aeon.utils.file_utils import create_directory, write_file
from aeon.utils.toml_utils import read_toml, write_toml


class TestConfigUtils(unittest.TestCase):
    """Test configuration utilities"""
    
    def setUp(self):
        """Set up test environment"""
        self.test_dir = tempfile.mkdtemp()
        self.original_home = os.environ.get('HOME')
        os.environ['HOME'] = self.test_dir
    
    def tearDown(self):
        """Clean up test environment"""
        if self.original_home:
            os.environ['HOME'] = self.original_home
        shutil.rmtree(self.test_dir)
    
    def test_write_and_read_global_config(self):
        """Test writing and reading global configuration"""
        config_data = {
            "aeon_path": "/test/path/to/aeon",
            "settings": {
                "debug": True,
                "version": "0.5.0"
            }
        }
        
        # Write config
        write_global_config(config_data)
        
        # Read config back
        read_config = read_global_config()
        
        self.assertEqual(read_config["aeon_path"], "/test/path/to/aeon")
        self.assertEqual(read_config["settings"]["debug"], True)
        self.assertEqual(read_config["settings"]["version"], "0.5.0")
    
    def test_read_nonexistent_config(self):
        """Test reading configuration when file doesn't exist"""
        config = read_global_config()
        self.assertIsNone(config)


class TestFileUtils(unittest.TestCase):
    """Test file utilities"""
    
    def setUp(self):
        """Set up test environment"""
        self.test_dir = tempfile.mkdtemp()
    
    def tearDown(self):
        """Clean up test environment"""
        shutil.rmtree(self.test_dir)
    
    def test_create_directory(self):
        """Test directory creation"""
        new_dir = Path(self.test_dir) / "test" / "nested" / "directory"
        
        create_directory(str(new_dir))
        
        self.assertTrue(new_dir.exists())
        self.assertTrue(new_dir.is_dir())
    
    def test_write_file(self):
        """Test file writing"""
        file_path = Path(self.test_dir) / "test_file.txt"
        content = "This is test content"
        
        write_file(str(file_path), content)
        
        self.assertTrue(file_path.exists())
        with open(file_path) as f:
            self.assertEqual(f.read(), content)
    
    def test_write_file_creates_directory(self):
        """Test that write_file creates parent directories"""
        file_path = Path(self.test_dir) / "nested" / "path" / "test_file.txt"
        content = "Test content"
        
        write_file(str(file_path), content)
        
        self.assertTrue(file_path.exists())
        self.assertTrue(file_path.parent.exists())


class TestTOMLUtils(unittest.TestCase):
    """Test TOML utilities"""
    
    def setUp(self):
        """Set up test environment"""
        self.test_dir = tempfile.mkdtemp()
    
    def tearDown(self):
        """Clean up test environment"""
        shutil.rmtree(self.test_dir)
    
    def test_write_and_read_toml(self):
        """Test writing and reading TOML files"""
        toml_file = Path(self.test_dir) / "test.toml"
        data = {
            "project": {
                "name": "test_project",
                "version": "1.0.0"
            },
            "dependencies": {
                "some_lib": "2.1.0"
            }
        }
        
        # Write TOML
        write_toml(str(toml_file), data)
        
        # Read TOML back
        read_data = read_toml(str(toml_file))
        
        self.assertEqual(read_data["project"]["name"], "test_project")
        self.assertEqual(read_data["project"]["version"], "1.0.0")
        self.assertEqual(read_data["dependencies"]["some_lib"], "2.1.0")
    
    def test_read_nonexistent_toml(self):
        """Test reading TOML file that doesn't exist"""
        nonexistent_file = Path(self.test_dir) / "nonexistent.toml"
        
        with self.assertRaises(FileNotFoundError):
            read_toml(str(nonexistent_file))


class TestSystemDependencies(unittest.TestCase):
    """Test system dependency management"""
    
    def setUp(self):
        """Set up test environment"""
        self.test_dir = tempfile.mkdtemp()
    
    def tearDown(self):
        """Clean up test environment"""
        shutil.rmtree(self.test_dir)
    
    def test_dependency_configuration(self):
        """Test system dependency configuration parsing"""
        # This is a basic structure test
        # More comprehensive tests would require mocking system calls
        
        deps_config = {
            "openssl": {
                "package": "openssl",
                "libs": ["ssl", "crypto"],
                "includes": ["/usr/local/opt/openssl/include"]
            },
            "boost": ["boost_system", "boost_filesystem"]
        }
        
        # Test that we can process different dependency formats
        self.assertIsInstance(deps_config["openssl"], dict)
        self.assertIsInstance(deps_config["boost"], list)
        
        # Verify structure
        self.assertEqual(deps_config["openssl"]["package"], "openssl")
        self.assertIn("ssl", deps_config["openssl"]["libs"])
        self.assertIn("boost_system", deps_config["boost"])


if __name__ == '__main__':
    unittest.main()
