# Store arguments passed to the script
args=("$@")
python3 ./Engine/Build/build_tool.py "${args[@]}"
