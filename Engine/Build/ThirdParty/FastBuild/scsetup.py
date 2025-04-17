# fetch fastbuild and unzip it in the correct folder for the platform
import os
from helpers import *

version = "1.14"

windows_address = f"https://www.fastbuild.org/downloads/v{version}/FASTBuild-Windows-x64-v{version}.zip"
linux_address = f"https://www.fastbuild.org/downloads/v{version}/FASTBuild-Linux-x64-v{version}.zip"
osx_address = f"https://www.fastbuild.org/downloads/v{version}/FASTBuild-OSX-x64+ARM-v{version}.zip"


def main() :
    fastbuild_archive = None

    # Fetch the FastBuild archive
    match get_target_platform() :
        case "Windows":
            fastbuild_archive = fetch_binary(windows_address, os.path.dirname(__file__))
        case "Linux":
            fastbuild_archive = fetch_binary(linux_address, os.path.dirname(__file__))
        case "OSX":
            fastbuild_archive = fetch_binary(osx_address, os.path.dirname(__file__))
        case _:
            raise Exception("Unknown platform %s" % get_target_platform())
        
    create_dir(os.path.join(os.path.dirname(__file__), get_target_platform()))
    extract_files(fastbuild_archive, os.path.join(os.path.dirname(__file__), get_target_platform()))