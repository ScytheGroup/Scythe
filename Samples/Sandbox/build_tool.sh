# Store the arguments passed to the script
args="$@"
# Set the path to the build tool
build_tool_path="../../Engine/Build/build_tool.py"
scythe_file_path="./Sandbox.scythe"
# scythe_file_path="./Build/project.scythe"
# Set the path to the python executable
python_executable="python"

$python_executable $build_tool_path $args $scythe_file_path