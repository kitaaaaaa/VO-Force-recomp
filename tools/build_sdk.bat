@echo off
rem Builds the patched ReXGlue SDK: build_sdk.bat <SDK source folder> <install folder>
rem Called by setup.ps1, which sets VSINSTALL and FORCE_TOOLS.
setlocal
call "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat" >nul
set "PATH=%FORCE_TOOLS%;%PATH%"
cd /d "%~1" || exit /b 1
cmake --preset win-amd64 -DCMAKE_INSTALL_PREFIX="%~2" || exit /b 1
cmake --build out\build\win-amd64 --config Release --target install || exit /b 1
