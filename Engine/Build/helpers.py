import os
import sys
import pathlib
import shutil

# Fetches remote files and saves them to disk
@staticmethod
def fetch_binary(remote_addr, local_dir) :
    import requests
    # if the file already exists, skip
    if os.path.exists(os.path.join(local_dir, os.path.basename(remote_addr))) :
        return os.path.join(local_dir, os.path.basename(remote_addr))

    # Fetch the remote file
    response = requests.get(remote_addr)
    if response.status_code != 200 :
        raise Exception("Failed to fetch %s" % remote_addr)
    # Save the file to disk
    local_path = os.path.join(local_dir, os.path.basename(remote_addr))
    with open(local_path, "wb") as f :
        f.write(response.content)
    return local_path


@staticmethod
def get_host_platform() -> str:
    if sys.platform == "win32":
        return "Windows"
    elif sys.platform == "linux":
        return "Linux"
    elif sys.platform == "darwin":
        return "OSX"
    else:
        raise Exception("Unknown platform %s" % sys.platform)

@staticmethod
def create_dir(path) :
    if not os.path.exists(path) :
        os.makedirs(path)


@staticmethod 
def extract_files(archive_path, dest_dir) :
    shutil.unpack_archive(archive_path, dest_dir)

###### Helper functions for running python modules ######

@staticmethod
def recursive_glob(root_dir, pattern) :
    return list(pathlib.Path(root_dir).rglob(pattern))
    

@staticmethod
def run_module(file_path, *args):
    import importlib.util
    spec = importlib.util.spec_from_file_location("module.name", file_path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    try:
        module.main(*args)
    except Exception as e:
        print("Error running %s: %s" % (file_path, e))


# Environment class to store the target platform and other environment variables
class Env :
    def __init__(self) :
        self.target_platform = get_host_platform()
        
    def get_os(self) :
        return self.target_platform
    
env = Env()

def get_target_platform() -> str:
    return env.target_platform


def run_dotnet_command(command, *args) :
    import subprocess
    subprocess.run(["dotnet", command, *args], check=True, stdout=sys.stdout)

def check_git_submodules_status() :
    import subprocess
    output = subprocess.run(["git", "submodule", "status"], check=True, stdout=sys.stdout)
    if output.stdout :
        # RUN git submodule update --init --recursive
        subprocess.run(["git", "submodule", "update", "--init", "--recursive"], check=True)

# returns the arguments to pass to the build tool as a dictionary
def get_buildtool_args(args) :
    new_args = dict()
    for arg in args :
        if arg.startswith("-") :
            arg = arg[1:]
            if "sources" in arg :
                if "sources" not in new_args :
                    new_args["sources"] = arg
                else :
                    new_args["sources"] += ', ' + arg
            if "=" in arg :
                key, value = arg.split("=")
                new_args[key] = value
            else :
                new_args[arg] = None
        elif os.path.exists(arg) :
            if (arg.endswith(".sharpmake.cs")) :
                if "sources" not in new_args :
                    new_args["sources"] = arg
                else :
                    new_args["sources"] += ', ' + arg
    return new_args

def get_args_strings(args : dict) :
    args_strings = []
    for key, value in args.items() :
        if value:
            args_strings.append("/" + key + "('" + value + "')")
        else:
            args_strings.append("/" + key)
    return args_strings

# legacy function to format the build tool arguments
def format_buildtool_args(args) :
    # sharpmake arguments have the format /arg1(value1) /arg2(value2) ...
    # But we support -arg[=value] format (=value is optional)
    # therefore we need to convert -arg=value to /arg(value)
    new_args = []
    for arg in args :
        if arg.startswith("-") :
            arg = arg[1:]
            if "=" in arg :
                arg, value = arg.split("=")
                new_args.append("/" + arg + "('" + value + "')")
            else :
                new_args.append("/" + arg)
    
    return new_args

def find_build_dir(dir) :
    for root, dirs, file in os.walk(dir) :
        for d in dirs :
            if d == "Build" :
                return os.path.join(root, d)
    return None

def extract_project_attributes(project_file) :
    with open(project_file, "r") as f :
        lines = f.readlines()
        attributes = dict()
        for line in lines :
            if not line.startswith("#") :
                key, value = line.split("=")
                key = key.strip()
                value = value.strip()
                attributes[key] = value
        return attributes

# todo : implement a more guided way to write project attributes
# def write_project_attributes(project_file, attributes) :
#     with open(project_file, "w") as f :
#         for key, value in attributes.items() :
#             f.write(key + ": " + value + "\n")