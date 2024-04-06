import sys
import os
from helpers import *


def main() :
    # if arguments contain -platform, use that as the target platform
    if "-platform" in sys.argv :
        env.target_platform = sys.argv[sys.argv.index("-platform") + 1]

    # run all setup.py files in subdirectories
    file_dir = os.path.dirname(os.path.realpath(__file__))
    setupfiles = recursive_glob(file_dir, "setup.py")
    print(setupfiles)

    for setupfile in setupfiles :
        print("Running %s" % setupfile)
        run_module(setupfile)

if __name__ == "__main__" :
    main()