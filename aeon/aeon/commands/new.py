import os
from aeon.utils.file_utils import create_directory, write_file

def new_project(project_name):
    # Create project directory
    project_dir = os.path.join(os.getcwd(), project_name)
    create_directory(project_dir)

    # Create subdirectories
    include_dir = os.path.join(project_dir, "Include")
    src_dir = os.path.join(project_dir, "src")
    create_directory(include_dir)
    create_directory(src_dir)

    # Create aeon.toml
    toml_content = f"""
[project]
name = "{project_name}"
version = "1.0.0"
description = "A new aeon project"
"""
    write_file(os.path.join(project_dir, "aeon.toml"), toml_content)

    # Create main.cpp
    main_cpp_content = """
#include "aeon/aeon.h"

using namespace aeon; // Bring aeon into scope
using namespace aeon::Http;

int main() {
    print("Starting aeon server..."); // Use print directly

    auto app = Server();

    // GET request handler
    app.get("/", [](Request& req, Response& res) {
        print("Handling GET request for /"); // Use print directly
        res.send("Hello from aeon!");
    });

    app.run(3000); // Start the server on port 3000
    
    return 0;
}
"""
    write_file(os.path.join(src_dir, "main.cpp"), main_cpp_content)

    print(f"Created new project: {project_name}")