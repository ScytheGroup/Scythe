# helper script to run sharpmake
import os
import subprocess
import sys
import helpers

# set the current directory to the script directory
# os.chdir(os.path.dirname(__file__))

# load engine root
def generate(project_file, args):
    project_dir = os.path.dirname(project_file)
    build_dir = helpers.find_build_dir(project_dir)
    project_attr = helpers.extract_project_attributes(project_file)

    # with open(os.path.join(os.path.dirname(build_dir), "project.scythe"), "r") as f:
    #     engine_root = f.read().strip()
    engine_root = project_attr['engine_dir']
    engine_root = os.path.abspath(engine_root)

    # fetch sharpmake path
    sharpmake_path = os.path.abspath(os.path.join(engine_root, "Engine/Build/ThirdParty/Sharpmake/bin/Sharpmake.Application"))
    if sys.platform == "win32":
        sharpmake_path += ".exe"

    # run sharpmake with the given arguments
    # fetch sharpmake.cs files in the current directory
    # engine_sharpmake_files = [f for f in os.listdir(os.getcwd()) if f.endswith(".sharpmake.cs")]
    sharpmake_files = ["'" + str(os.path.normpath(os.path.join(build_dir, f))) + "'" for f in os.listdir(build_dir) if f.endswith(".sharpmake.cs")]
    engine_build_dir = os.path.join(engine_root, "Engine", "Build")
    if (build_dir != engine_build_dir):
        sharpmake_files += ["'" + str(os.path.normpath(os.path.join(engine_build_dir, f))) + "'" for f in os.listdir(engine_build_dir) if f.endswith("sharpmake.cs")]

    # if the path delimiters are \\, replace them with \\\\ for windows
    if sys.platform == "win32":
        sharpmake_files = [f.replace("\\", "\\\\") for f in sharpmake_files]

    sharpmake_files = ", ".join(sharpmake_files)


    sources_string = "/sources(" + sharpmake_files + ")"

    args_dict = helpers.get_buildtool_args(args)
    if "sources" in args_dict:
        sources_string = "/sources('" + args_dict["sources"] + "')"
        del args_dict["sources"]

    formatted_args_string = helpers.get_args_strings(args_dict)
    print(" ".join([sharpmake_path, sources_string, *formatted_args_string]))
    # todo: add support for a configuration file to store the default arguments
    subprocess.run(" ".join([sharpmake_path, *formatted_args_string, sources_string]), check=True)


def extract_scythe_file_path(args):
    files = []
    for i in range(len(args)):
        if os.path.exists(args[i]):
            # if extension is .scythe
            if args[i].endswith(".scythe"):
                files.append(args[i])
    if len(files) != 1:
        print(".scythe file either not found or to many were provided. Exiting...")
        sys.exit(1)

    args.remove(files[0])
    return os.path.abspath(files[0])


def main():
    if len(sys.argv) < 2:
        print("Usage: <script_path> <project_dir> [args]")
        engine_scythe_file = os.path.abspath(os.path.join(os.path.dirname(os.path.dirname(__file__)), "project.scythe"))
        args = [engine_scythe_file]
    else :
        args = sys.argv[1:]

    project_file = extract_scythe_file_path(args)
    if project_file is None:
        print("project.scythe file not found")
        sys.exit(1)
    

    generate(project_file, args) 

if __name__ == "__main__" :
    main()
