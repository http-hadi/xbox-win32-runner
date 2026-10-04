@echo off
rem Desktop build of the same probe source, so the probe LOGIC can be validated
rem on this PC before it is deployed to the console. If this passes locally and
rem fails on the console, the difference is the console environment, not the code.
setlocal
set VSDIR=C:\Program Files\Microsoft Visual Studio\2022\Community
set PROBEDIR=C:\Users\Administrator\Documents\deepseek-harness\default-workspace\probe
set OUTDIR=%PROBEDIR%\out_desktop
if not exist "%OUTDIR%" mkdir "%OUTDIR%"

call "%VSDIR%\VC\Auxiliary\Build\vcvarsall.bat" x64
if errorlevel 1 ( echo VCVARS_FAILED & exit /b 1 )

cd /d "%OUTDIR%"
echo === compiling (desktop) ===
cl /nologo /c /EHsc /MD /O2 /std:c++17 /DUNICODE /D_UNICODE ^
   /Fo"%OUTDIR%\d3d11probe.obj" "%PROBEDIR%\d3d11probe.cpp"
if errorlevel 1 ( echo COMPILE_FAILED & exit /b 1 )

cl /nologo /c /EHsc /MD /O2 /std:c++17 /DUNICODE /D_UNICODE ^
   /Fo"%OUTDIR%\testmain.obj" "%PROBEDIR%\testmain.cpp"
if errorlevel 1 ( echo COMPILE_FAILED & exit /b 1 )

echo === linking (desktop) ===
link /NOLOGO /SUBSYSTEM:CONSOLE /OUT:"%OUTDIR%\probe_test.exe" ^
   "%OUTDIR%\d3d11probe.obj" "%OUTDIR%\testmain.obj" ^
   d3d11.lib dxgi.lib d3dcompiler.lib
if errorlevel 1 ( echo LINK_FAILED & exit /b 1 )

echo === running ===
"%OUTDIR%\probe_test.exe"
echo RUN_EXIT=%ERRORLEVEL%
echo BUILD_DESKTOP_OK
