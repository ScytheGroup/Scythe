####TODO ADD CODE FOR 32 BIT SUPPORT#####
import os
import subprocess
import shutil
import sys
from pathlib import Path


# Constants
VCPKG_LIST = ""
VCPKG_TMPFOLDER = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'tmp', 'vcpkg')
VCPKG_OPERATIONS_FOLDER = f"{Path.home()}/Documents/.vcpkg/"
VCPKG_INSTALL_FOLDER = f"{VCPKG_OPERATIONS_FOLDER}/install/"
VCPKG_BUILD_FOLDER = f"{VCPKG_OPERATIONS_FOLDER}/build/"


def run_command(command):
    result = subprocess.run(command, shell=True)
    if result.returncode != 0:
        sys.exit(result.returncode)

def get_vcpkg():
    if not os.path.exists(VCPKG_TMPFOLDER):
        clone_vcpkg()
    else:
        pull_vcpkg()

def clone_vcpkg():
    print("Cloning vcpkg")
    run_command(f"git clone https://github.com/microsoft/vcpkg.git {VCPKG_TMPFOLDER}")

def pull_vcpkg():
    print("Pulling latest vcpkg")
    run_command(f"git -C {VCPKG_TMPFOLDER} pull")

def build_vcpkg():
    print("Building vcpkg")
    run_command(f"{VCPKG_TMPFOLDER}/bootstrap-vcpkg.bat")

def install_vcpkg_packages():
    # print(f"Installing {package_str}")
    # {package_str}
    
    # get os
    str = os.name
    os_name = os.name
    if os_name == "nt":
        str = "windows"
    elif os_name == "posix":
        str = "linux"
    else:
        print("Unsupported OS")
        return
    
    # get arch x64 or arm
    arch = os.environ.get("PROCESSOR_ARCHITECTURE")
    if arch == "AMD64":
        arch = "x64"
    elif arch == "ARM64":
        arch = "arm64"
    else:
        print("Unsupported architecture")

    str = f"{arch}-{str}-static"

    run_command(f"{VCPKG_TMPFOLDER}/vcpkg.exe install --x-buildtrees-root={VCPKG_BUILD_FOLDER} --x-install-root={VCPKG_INSTALL_FOLDER} --clean-after-build --triplet {str}")

def generate_vcpkg_export():
    print("Generating vcpkg export")
    package_str = " ".join(VCPKG_LIST)
    # {package_str}
    export_as_zip = False
    format = "raw"
    for arg in sys.argv:
        if arg == "-zip":
            export_as_zip = True
            format = "zip"
            print("Exporting vcpkg as zip")
            break
        elif arg == "-7z":
            export_as_zip = True
            format = "7zip"
            print("Exporting vcpkg as 7z")
            break
    # run_command(f"{VCPKG_TMPFOLDER}/vcpkg.exe export --raw --output=vcpkg --output-dir=extern --x-install-root={VCPKG_TMPFOLDER}/installed")
    run_command(f"{VCPKG_TMPFOLDER}/vcpkg.exe export --{format} --output=vcpkg --output-dir=extern --x-install-root={VCPKG_INSTALL_FOLDER}")

def main():
    filedir = os.path.dirname(__file__)
    # run update-vcpkg.bat
    os.chdir(filedir)
    
    if os.path.exists(os.path.join(filedir, "extern/vcpkg")) :
        shutil.rmtree(os.path.join(filedir, "extern/vcpkg"))

    # read the vcpkg list from packages.txt
    with open(os.path.join(filedir, "packages.txt"), "r") as f:
        global VCPKG_LIST
        VCPKG_LIST = f.read().splitlines()

    force_update_arg = False
    for arg in sys.argv:
        if arg == "-force-update":
            force_update_arg = True
            print("Forcing update update of vcpkg deps")
            break
    
    # if extern/vcpkg.zip exists, extract it
    if (force_update_arg != True) and os.path.exists((os.path.join(filedir, "extern/vcpkg.zip"))):
        print("zip package found!\nExtracting vcpkg.zip")
        shutil.unpack_archive(os.path.join(filedir, "extern/vcpkg.zip"), os.path.join(filedir, "extern"))
        return

    print("Running vcpkg setup")    

    get_vcpkg()
    build_vcpkg()
    install_vcpkg_packages()
    generate_vcpkg_export()

    shutil.rmtree(VCPKG_OPERATIONS_FOLDER)
    
if __name__ == "__main__":
    main()