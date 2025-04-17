import helpers
import os
import sys
import shutil

# SmallFile where the first argument is a project path.
# if the -create argument is not passed try and find a *.scythe file in the project path
# if the -create argument is passed create a new project folder using the template project folder in engine_root/Template

args_with_paths = ["-name"]

template_path = os.path.normpath(os.path.dirname(os.path.realpath(__file__)) + "../../../Template")
true_engine_path = os.path.normpath(os.path.dirname(os.path.realpath(__file__)) + "../../..")

def get_formatted_project_file_str(project_name):
    # get the .scythe content in the template folder
    content = None
    with open(os.path.join(template_path, "[project_name].scythe"), "r") as f:
        content = f.read()

    # replace the project name and engine root
    content = content.replace("[project_name]", project_name)
    content = content.replace("[engine_dir]", true_engine_path)
    return content

def get_formatted_bat_file_str(project_name):
    # get the .bat content in the template folder
    content = None
    with open(os.path.join(template_path, "build_tool.bat"), "r") as f:
        content = f.read()

    # replace the project name and engine root
    content = content.replace("[project_name]", project_name)
    content = content.replace("[engine_dir]", true_engine_path)
    return content

def get_formatted_sh_file_str(project_name):
    # get the .sh content in the template folder
    content = None
    with open(os.path.join(template_path, "build_tool.sh"), "r") as f:
        content = f.read()

    # replace the project name and engine root
    content = content.replace("[project_name]", project_name)
    content = content.replace("[engine_dir]", true_engine_path)
    return content


def find_path_in_args(args):
    prev_arg = None
    for arg in args:
        if arg.startswith("-"):
            prev_arg = arg
            continue
        if os.path.exists(arg) and not prev_arg in args_with_paths:
            return arg
        prev_arg = arg
    return None


def validate_engine_root(engine_root):
    if engine_root != true_engine_path:
        engine_root = true_engine_path
    return engine_root


def refresh_project_file(project_path):
    project_path = os.path.abspath(project_path)
    # find file finish with .scythe but only in the project path
    scythe_file = [f for f in os.listdir(project_path) if f.endswith(".scythe")]

    if not scythe_file:
        print("Could not find project file")
        print("Generate a file? (y/n)")
        answer = input()
        if answer == "y":
            # Only copy the template project file to the project path
            # template_path = os.path.normpath(os.path.dirname(os.path.realpath(__file__)) + "../../../Template")
            if "-name" in sys.argv:
                project_name = sys.argv[sys.argv.index("-name") + 1]
            else:
                project_name = os.path.basename(project_path)

            scythe_file = os.path.join(project_path,project_name + ".scythe")
            with open(scythe_file, "w") as f:
                f.write(get_formatted_project_file_str(project_name))
        else : 
            return
        
        file_args = helpers.extract_project_attributes(scythe_file)
        engine_root = file_args["engine_dir"]
    else:
        scythe_file = os.path.join(project_path, scythe_file[0])

        print("Found project file: " + scythe_file)
        print("Refreshing...")

        file_args = helpers.extract_project_attributes(scythe_file)
        engine_root = file_args["engine_dir"]

        file_args["engine_dir"] = validate_engine_root(engine_root)

        with open(scythe_file, "w") as f:
            for key in file_args:
                f.write(key + "=" + file_args[key] + "\n")
    
    # find build_tool files in the project path
    print("Refreshing build_tool files...")
    bat_file = os.path.join(project_path, "build_tool.bat")
    with open(bat_file, "w") as f:
        f.write(get_formatted_bat_file_str(file_args["name"]))
    sh_file = os.path.join(project_path, "build_tool.sh")
    with open(sh_file, "w") as f:
        f.write(get_formatted_sh_file_str(file_args["name"]))

    print("project refreshed!")
    
def create_project(project_dest, args):
    # Check if the project folder contains files
    if not os.path.exists(project_dest):
        # Create the project folder
        os.mkdir(project_dest)

    if "-name" in args:
        project_name = args[args.index("-name") + 1]
    else:
        print("Enter new project name: ")
        project_name = input()

    # Copy the template project folder to the project path
    # template_path = os.path.normpath(os.path.dirname(os.path.realpath(__file__)) + "../../../Template")
    shutil.copytree(template_path, os.path.join(project_dest, project_name))
    os.chdir(os.path.join(project_dest, project_name))

    # In all files that can be found in that new dir and any of its subdirs,
    #  replace the string "[project_name]" with the project name and "[engine_dir]" with the engine root
    files = helpers.recursive_glob(os.getcwd(), "*")
    for file in files:
        try :
            # Start by the name of the file
            filestr = str(file)
            if "[project_name]" in filestr:
                new_file = filestr.replace("[project_name]", project_name)
                os.rename(file, new_file)
                file = new_file
        
            if os.path.isfile(file) and os.access(file, os.R_OK):
                with open(file, "r") as f:
                    file_data = f.read()
                    file_data = file_data.replace("[project_name]", project_name)
                    file_data = file_data.replace("[engine_dir]", os.path.normpath(os.path.dirname(os.path.realpath(__file__)) + "../../.."))
                with open(file, "w") as f:
                    f.write(file_data)
        except :
            pass

def write_help():
    help_message = """
Usage: python project_gen.py [options] [project_path]

Options:
  -help          Show this help message and exit.
  -create, -new  Create a new project folder using the template project folder.
  -name NAME     Specify the name of the project.

Arguments:
  project_path   Path to the project directory where operations will be performed.
                 If not provided, the script will look for the project path in the arguments.

Description:
This script helps manage project files by either creating a new project or refreshing an existing one.

When using -create or -new:
1. The script will create a new project folder using the template located at the engine's Template directory.
2. You must specify a project name using the -name option.
   Example: python project_gen.py -create -name MyProject

When not using -create or -new:
1. The script will refresh an existing project directory.
2. It will search for a .scythe file in the specified project path or ask if you want to generate one if none is found.
3. The script will update the .scythe file and refresh build_tool.bat and build_tool.sh files with the correct project and engine directory information.

Examples:
1. To create a new project named "MyProject":
   python project_gen.py -create -name MyProject

2. To refresh an existing project located at /path/to/project:
   python project_gen.py /path/to/project

Note:
- If no arguments are provided, the script will not perform any operations.
- Make sure the project path and the template directory are correctly set up before running the script.
    """
    print(help_message)

def main():
    args = sys.argv[1:]
    if not args or "-help" in args:
        write_help()
        sys.exit()

    project_path = find_path_in_args(args)
    if "-create" or "-new" in args:
        create_project(project_path, args)
    else:
        refresh_project_file(project_path)


if __name__ == "__main__":
    main()
