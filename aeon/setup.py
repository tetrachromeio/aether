from setuptools import setup, find_packages

setup(
    name="aeon",
    version="1.0.0",
    packages=find_packages(),
    entry_points={
        "console_scripts": [
            "aeon = aeon.cli:main",
        ],
    },
    install_requires=["toml"],
)