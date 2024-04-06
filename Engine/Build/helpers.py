import requests
import os
import sys
import pathlib
import shutil

# Fetches remote files and saves them to disk
@staticmethod
def fetch_binary(remote_addr, local_dir) :
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
def run_module(file_path, *args) :
    import importlib.util
    spec = importlib.util.spec_from_file_location("module.name", file_path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.main(*args)


# Environment class to store the target platform and other environment variables
class Env :
    def __init__(self) :
        self.target_platform = get_host_platform()
        
    def get_os(self) :
        return self.target_platform
    
env = Env()

def get_target_platform() -> str:
    return env.target_platform