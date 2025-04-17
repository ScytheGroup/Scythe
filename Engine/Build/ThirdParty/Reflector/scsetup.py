from helpers import *


def get_dotnet_platform() -> str:
    if get_host_platform() == "Windows":
        return "win"
    elif get_host_platform() == "Linux":
        return "linux"
    elif get_host_platform() == "OSX":
        return "osx"
    else:
        raise Exception("Unknown platform %s" % get_host_platform())

def main():
    check_git_submodules_status()
    
    filedir = os.path.dirname(__file__)
    run_dotnet_command("publish", "--no-self-contained", "-c", "Release", "-r", get_dotnet_platform() + '-x64', "-o", os.path.join(filedir, "bin"), os.path.join(filedir, "ScytheReflector", "ScytheReflector", "ScytheReflector.csproj"))

    # if bin/db exists, delete it
    dbdir = os.path.join(filedir, "bin", "db")
    if os.path.exists(dbdir):
        shutil.rmtree(dbdir)