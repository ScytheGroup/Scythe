@REM Store the arguments passed to the script
set args=%*
@REM Set the path to the build tool
set build_tool_path=..\..\Engine\Build\build_tool.py
set scythe_file_path=.\Sandbox.scythe
@REM set scythe_file_path=.\Build\project.scythe
@REM Set the path to the python executable
set python_executable=python

%python_executable% %build_tool_path% %args% %scythe_file_path%