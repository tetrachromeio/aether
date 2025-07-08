"""
Integration tests for Aeon package manager
Tests complete workflows and interactions between components
"""

import unittest
import tempfile
import shutil
import os
import subprocess
from pathlib import Path
import sys

# Add the aeon package to the Python path
sys.path.insert(0, str(Path(__file__).parent.parent / "aeon"))


class TestAeonIntegration(unittest.TestCase):
    """Integration tests for complete Aeon workflows"""
    
    def setUp(self):
        """Set up test environment"""
        self.test_dir = tempfile.mkdtemp()
        self.original_cwd = os.getcwd()
        os.chdir(self.test_dir)
    
    def tearDown(self):
        """Clean up test environment"""
        os.chdir(self.original_cwd)
        shutil.rmtree(self.test_dir)
    
    def test_complete_project_workflow(self):
        """Test complete project creation and setup workflow"""
        project_name = "integration_test_project"
        
        # Import here to avoid path issues
        from aeon.commands.new import new_project
        
        # Step 1: Create new project
        new_project(project_name, "blank")
        
        # Step 2: Verify project was created correctly
        project_path = Path(self.test_dir) / project_name
        self.assertTrue(project_path.exists())
        
        # Step 3: Change to project directory
        os.chdir(project_path)
        
        # Step 4: Verify all expected files exist
        expected_files = [
            "aeon.toml",
            "src/main.cpp",
            "Include"
        ]
        
        for file_path in expected_files:
            self.assertTrue((project_path / file_path).exists(), f"Missing {file_path}")
        
        # Step 5: Verify aeon.toml content is valid
        import toml
        with open("aeon.toml") as f:
            config = toml.load(f)
        
        self.assertEqual(config["project"]["name"], project_name)
        self.assertIn("version", config["project"])
        self.assertIn("description", config["project"])
    
    def test_aether_project_workflow(self):
        """Test Aether-specific project creation workflow"""
        project_name = "aether_integration_test"
        
        from aeon.commands.new import new_project
        
        # Create Aether project
        new_project(project_name, "aether")
        
        project_path = Path(self.test_dir) / project_name
        os.chdir(project_path)
        
        # Verify Aether-specific structure
        aether_specific_files = [
            "src/models/admin.toml",
            "src/models/user.toml", 
            "public"
        ]
        
        for file_path in aether_specific_files:
            self.assertTrue((project_path / file_path).exists(), f"Missing {file_path}")
        
        # Verify system dependencies are configured
        import toml
        with open("aeon.toml") as f:
            config = toml.load(f)
        
        self.assertIn("systemDependencies", config)
        self.assertIn("openssl", config["systemDependencies"])
    
    def test_project_toml_validation(self):
        """Test that generated TOML files are valid and well-formed"""
        from aeon.commands.new import new_project
        
        # Test both templates
        for template in ["blank", "aether"]:
            project_name = f"toml_test_{template}"
            new_project(project_name, template)
            
            project_path = Path(self.test_dir) / project_name
            toml_file = project_path / "aeon.toml"
            
            # Verify TOML can be parsed
            import toml
            with open(toml_file) as f:
                config = toml.load(f)
            
            # Verify required sections exist
            self.assertIn("project", config)
            self.assertIn("name", config["project"])
            self.assertIn("version", config["project"])
            self.assertIn("description", config["project"])
            
            # Verify project name was set correctly
            self.assertEqual(config["project"]["name"], project_name)


class TestBuildSystemIntegration(unittest.TestCase):
    """Integration tests for the build system"""
    
    def setUp(self):
        """Set up test environment"""
        self.test_dir = tempfile.mkdtemp()
        self.original_cwd = os.getcwd()
        os.chdir(self.test_dir)
    
    def tearDown(self):
        """Clean up test environment"""
        os.chdir(self.original_cwd)
        shutil.rmtree(self.test_dir)
    
    def test_build_cache_functionality(self):
        """Test that build cache system works correctly"""
        from aeon.commands.new import new_project
        
        project_name = "build_cache_test"
        new_project(project_name, "blank")
        
        project_path = Path(self.test_dir) / project_name
        os.chdir(project_path)
        
        # Create build directory structure
        build_dir = project_path / "build"
        build_dir.mkdir(exist_ok=True)
        
        # Test cache file creation and loading
        from aeon.commands.build import load_build_cache, save_build_cache
        
        cache_file = build_dir / "build_cache.json"
        
        # Test empty cache
        cache = load_build_cache(cache_file)
        self.assertEqual(cache, {})
        
        # Test cache saving and loading
        test_cache = {
            "test_file.cpp": {
                "hash": "abc123",
                "compiled_at": 1234567890
            }
        }
        
        save_build_cache(cache_file, test_cache)
        loaded_cache = load_build_cache(cache_file)
        
        self.assertEqual(loaded_cache, test_cache)


if __name__ == '__main__':
    unittest.main()
