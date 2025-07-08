from setuptools import setup, find_packages

setup(
    name="aeon",
    version="0.5.0",
    packages=find_packages(),
    include_package_data=True,
    entry_points={
        "console_scripts": [
            "aeon = aeon.cli:main",
        ],
    },
    install_requires=["toml","tqdm","requests","gitpython","tomlkit"],
)