# Depending on distro, you may need to run this script with in a venv

activate() {
    source ./venv/bin/activate
}
git submodule update --init --recursive
cd ./Engine/Build
python3 -m venv venv
activate
pip3 install -r requirements.txt --no-cache-dir && python3 setupall.py
rm -rf ./venv