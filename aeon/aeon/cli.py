import argparse
from aeon.commands.new import new_project
from aeon.commands.build import build_project
from aeon.commands.run import run_project
from aeon.commands.link import link_aeon

def main():
    parser = argparse.ArgumentParser(description="aeon Package Manager")
    subparsers = parser.add_subparsers(dest="command", help="Available commands")

    # aeon new <project>
    new_parser = subparsers.add_parser("new", help="Create a new project")
    new_parser.add_argument("project_name", help="Name of the project")

    # aeon build
    subparsers.add_parser("build", help="Build the project")

    # aeon run
    subparsers.add_parser("run", help="Run the project")

    # aeon link <path_to_aeon>
    link_parser = subparsers.add_parser("link", help="Link the aeon library")
    link_parser.add_argument("aeon_path", help="Path to the aeon library")


    args = parser.parse_args()

    if args.command == "new":
        new_project(args.project_name)
    elif args.command == "build":
        build_project()
    elif args.command == "run":
        run_project()
    elif args.command == "link":
        link_aeon(args.aeon_path)
    else:
        parser.print_help()

if __name__ == "__main__":
    main()