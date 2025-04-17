cd .\Engine\Build
git submodule update --init --recursive
python -m venv venv
@rem the args
set args=%*
.\venv\Scripts\activate.bat && pip install -r requirements.txt --no-cache-dir && python setupall.py %args%

deactivate

rd /s /q venv
cd ..\..