"""
Test runner for Aeon package manager test suite
Provides test discovery, coverage reporting, and test execution
"""

import unittest
import sys
import os
from pathlib import Path

# Add the aeon package to the Python path for testing
test_dir = Path(__file__).parent
aeon_dir = test_dir.parent / "aeon"
sys.path.insert(0, str(aeon_dir))


def run_tests(verbosity=2, pattern="test_*.py"):
    """
    Run the complete test suite
    
    Args:
        verbosity (int): Test output verbosity level
        pattern (str): Pattern to match test files
    
    Returns:
        bool: True if all tests passed, False otherwise
    """
    # Discover and run tests
    loader = unittest.TestLoader()
    start_dir = str(test_dir)
    suite = loader.discover(start_dir, pattern=pattern)
    
    # Run tests
    runner = unittest.TextTestRunner(verbosity=verbosity)
    result = runner.run(suite)
    
    # Print summary
    print(f"\n{'='*50}")
    print(f"Test Summary:")
    print(f"Tests run: {result.testsRun}")
    print(f"Failures: {len(result.failures)}")
    print(f"Errors: {len(result.errors)}")
    print(f"Skipped: {len(result.skipped) if hasattr(result, 'skipped') else 0}")
    print(f"{'='*50}")
    
    # Return success status
    return result.wasSuccessful()


def run_coverage():
    """
    Run tests with coverage reporting
    Requires coverage.py to be installed: pip install coverage
    """
    try:
        import coverage
    except ImportError:
        print("Coverage.py not installed. Install with: pip install coverage")
        print("Running tests without coverage...")
        return run_tests()
    
    # Start coverage
    cov = coverage.Coverage(source=[str(aeon_dir)])
    cov.start()
    
    try:
        # Run tests
        success = run_tests()
        
        # Stop coverage and generate report
        cov.stop()
        cov.save()
        
        print(f"\n{'='*50}")
        print("Coverage Report:")
        print(f"{'='*50}")
        cov.report()
        
        # Generate HTML report
        html_dir = test_dir / "coverage_html"
        cov.html_report(directory=str(html_dir))
        print(f"\nHTML coverage report generated in: {html_dir}")
        
        return success
        
    except Exception as e:
        cov.stop()
        print(f"Error running coverage: {e}")
        return False


def main():
    """Main test runner entry point"""
    import argparse
    
    parser = argparse.ArgumentParser(description="Aeon Test Suite Runner")
    parser.add_argument("--coverage", "-c", action="store_true", 
                       help="Run tests with coverage reporting")
    parser.add_argument("--pattern", "-p", default="test_*.py",
                       help="Pattern to match test files (default: test_*.py)")
    parser.add_argument("--verbose", "-v", action="count", default=2,
                       help="Increase verbosity level")
    
    args = parser.parse_args()
    
    print("Aeon Package Manager Test Suite")
    print(f"{'='*50}")
    
    if args.coverage:
        success = run_coverage()
    else:
        success = run_tests(verbosity=args.verbose, pattern=args.pattern)
    
    # Exit with appropriate code
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
