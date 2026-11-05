@echo off
rem Build Force-recomp on Windows.
rem
rem   build.bat [debug|release|relwithdebinfo]   (default: release)
rem
rem Needs: Visual Studio 2022 (C++ workload, for the MSVC STL + Windows SDK),
rem Clang 20+, CMake 3.25+, Ninja, and the ReXGlue SDK v0.10.0.
rem
rem Portable copies in ..\tools\ (llvm, cmake, rexsdk\win-amd64) are used when
rem present. Set REXSDK to point at a different SDK install prefix.
setlocal EnableDelayedExpansion

set "ROOT=%~dp0"
set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=release"
set "PRESET=win-amd64-%CONFIG%"

if exist "%ROOT%..\tools\llvm\bin\clang.exe" set "PATH=%ROOT%..\tools\llvm\bin;%PATH%"
if exist "%ROOT%..\tools\cmake\bin\cmake.exe" set "PATH=%ROOT%..\tools\cmake\bin;%PATH%"
rem Prefer the SDK built from source (tools\rexglue-sdk, work\sdkbuild.bat).
if "%REXSDK%"=="" if exist "%ROOT%..\tools\rexsdk-custom" set "REXSDK=%ROOT%..\tools\rexsdk-custom"
if "%REXSDK%"=="" if exist "%ROOT%..\tools\rexsdk\win-amd64" set "REXSDK=%ROOT%..\tools\rexsdk\win-amd64"

rem Load the MSVC environment (headers/libs for clang's MSVC target).
rem (%ProgramFiles(x86)% contains ")", so avoid blocks and use !VSWHERE!.)
if not "%VCToolsInstallDir%"=="" goto :have_msvc
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq delims=" %%i in (`"!VSWHERE!" -latest -products * -property installationPath`) do set "VSINSTALL=%%i"
if not exist "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat" goto :no_vs
call "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat" >nul
:have_msvc

rem Tools downloaded by tools\setup.ps1 come first, ahead of any in Visual Studio.
if not "%FORCE_TOOLS%"=="" set "PATH=%FORCE_TOOLS%;%PATH%"

pushd "%ROOT%"
cmake --preset %PRESET% -DCMAKE_PREFIX_PATH="%REXSDK%" || goto :fail
cmake --build out\build\%PRESET% || goto :fail
popd
echo.
echo Built: %ROOT%out\build\%PRESET%\force.exe
exit /b 0

:fail
popd
exit /b 1

:no_vs
echo Visual Studio 2022 with the C++ workload was not found.
exit /b 1
