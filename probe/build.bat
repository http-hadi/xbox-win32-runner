@echo off
setlocal
set VSDIR=C:\Program Files\Microsoft Visual Studio\2022\Community
set PROBEDIR=C:\Users\Administrator\Documents\deepseek-harness\default-workspace\probe
set OUTDIR=%PROBEDIR%\out
if not exist "%OUTDIR%" mkdir "%OUTDIR%"

call "%VSDIR%\VC\Auxiliary\Build\vcvarsall.bat" x64 uwp
if errorlevel 1 (
  echo VCVARS_FAILED
  exit /b 1
)

cd /d "%OUTDIR%"

echo === compiling ===
rem /MD not /MT: the UWP/Store environment ships msvcprt.lib (DLL CRT),
rem there is no static libcpmt.lib, and the app already depends on VCLibs.
cl /nologo /c /EHsc /MD /O2 /std:c++17 /DUNICODE /D_UNICODE /W3 ^
   /Fo"%OUTDIR%\d3d11probe.obj" "%PROBEDIR%\d3d11probe.cpp"
if errorlevel 1 (
  echo COMPILE_FAILED
  exit /b 1
)

echo === linking ===
rem WindowsApp.lib is the UWP umbrella import lib: without it the CRT's
rem WinRT init and a few kernel32 forwards have no import library.
link /NOLOGO /DLL /OUT:"%OUTDIR%\d3d11probe.dll" ^
   "%OUTDIR%\d3d11probe.obj" d3d11.lib dxgi.lib d3dcompiler.lib WindowsApp.lib
if errorlevel 1 (
  echo LINK_FAILED
  exit /b 1
)

echo === result ===
dir "%OUTDIR%\d3d11probe.dll"
echo BUILD_OK
