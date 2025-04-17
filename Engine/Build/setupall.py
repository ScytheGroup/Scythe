import sys
import os
from helpers import *


def main() :
    # if arguments contain -platform, use that as the target platform
    if "-platform" in sys.argv :
        env.target_platform = sys.argv[sys.argv.index("-platform") + 1]

    # Update submodules
    check_git_submodules_status()

    # run all setup.py files in subdirectories
    file_dir = os.path.dirname(os.path.realpath(__file__))
    setupfiles = recursive_glob(file_dir, "scsetup.py")
    print(setupfiles)

    # remove files contained in a venv directory
    for setupfile in setupfiles :
        abs_path = str(os.path.abspath(setupfile))
        if abs_path.find("venv") != -1:
            continue
        print("Running %s" % setupfile)
        run_module(setupfile)
        

    print("Setup complete")
    engine_dir = os.path.dirname(file_dir)
    # print grandparent directory in a local file called project.scythe
    if os.path.exists(os.path.join(engine_dir, "project.scythe")) :
        os.remove(os.path.join(engine_dir, "project.scythe"))
    with open(os.path.join(engine_dir, "project.scythe"), "w") as f :
        # For now the file format will be key=value. Eventually maybe move to json format
        f.write('engine_dir=' + os.path.dirname(engine_dir))

if __name__ == "__main__" :
    main()