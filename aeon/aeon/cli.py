import argparse
from aeon.commands import new_project, build_project, run_project, link_aeon, makemigrations, migrate
from aeon.commands.build import clean_build
from aeon.commands.dev import dev_server
from aeon.commands.deps import show_system_dependencies, check_system_dependencies

def main():
    parser = argparse.ArgumentParser(description="aeon Package Manager")
    subparsers = parser.add_subparsers(dest="command", help="Available commands")

    # aeon new <project_name> [template]
    new_parser = subparsers.add_parser("new", help="Create a new project")
    new_parser.add_argument("project_name", help="Name of the project")
    new_parser.add_argument("template", nargs="?", default="blank", help="Template to use (default: blank)")

    # aeon build
    subparsers.add_parser("build", help="Build the project")

    # aeon dev
    subparsers.add_parser("dev", help="Start development server with hot reload")

    # aeon clean
    subparsers.add_parser("clean", help="Clean build artifacts and cache")

    # aeon run
    subparsers.add_parser("run", help="Run the project")

    # aeon link <path_to_aeon>
    link_parser = subparsers.add_parser("link", help="Link the aeon library")
    link_parser.add_argument("aeon_path", help="Path to the aeon library")

    # aeon makemigrations
    subparsers.add_parser("makemigrations", help="Create new migrations based on changes")

    # aeon migrate
    subparsers.add_parser("migrate", help="Apply migrations to the database")

    # aeon deps
    deps_parser = subparsers.add_parser("deps", help="Manage system dependencies")
    deps_subparsers = deps_parser.add_subparsers(dest="deps_command", help="Dependencies commands")
    deps_subparsers.add_parser("show", help="Show system dependencies configuration")
    deps_subparsers.add_parser("check", help="Check if system dependencies can be resolved")

    #superuser creation command
    subparsers.add_parser("createsuperuser", help="Create a superuser for the application")
    from aeon.commands.createsuperuser import create_superuser
                    

    args = parser.parse_args()

    if args.command == "new":
        new_project(args.project_name, args.template)
    elif args.command == "build":
        build_project()
    elif args.command == "dev":
        dev_server()
    elif args.command == "clean":
        clean_build()
    elif args.command == "run":
        run_project()
    elif args.command == "link":
        link_aeon(args.aeon_path)
    elif args.command == "makemigrations":
        makemigrations()
    elif args.command == "migrate":
        migrate()
    elif args.command == "deps":
        if args.deps_command == "show":
            show_system_dependencies()
        elif args.deps_command == "check":
            check_system_dependencies()
        else:
            deps_parser.print_help()
    elif args.command == "createsuperuser":
        create_superuser()
    else:
        parser.print_help()

if __name__ == "__main__":
    main()
