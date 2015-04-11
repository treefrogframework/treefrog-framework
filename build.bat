@ECHO OFF
@setlocal

::
:: Edit this line to run the batch file for Qt environment.
::
::call "C:\Qt\Qt5.3.2-mingw\5.3\mingw482_32\bin\qtenv2.bat"
::call "C:\Qt\Qt5.4.1-mingw\5.4\mingw491_32\bin\qtenv2.bat"
::call "C:\Qt\Qt5.3.2\5.3\msvc2013_64\bin\qtenv2.bat"
call "C:\Qt\Qt5.4.1\5.4\msvc2013_64_opengl\bin\qtenv2.bat"

call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" amd64


if "%Platform%" == "X64" (
  set MAKE=nmake
  set CL=/MP
) else if not "%DevEnvDir%" == "" (
  set MAKE=nmake
  set CL=/MP
) else (
  set MAKE=mingw32-make -j4
)

::
:: Compile and Install
::
cd /D %~dp0

call configure.bat --enable-debug
if ERRORLEVEL 1 goto :error

cd src
%MAKE% install
if ERRORLEVEL 1 goto :error

cd ..\tools
%MAKE% install
if ERRORLEVEL 1 goto :error

cd ..
call configure.bat
if ERRORLEVEL 1 goto :error

cd src
%MAKE% install
if ERRORLEVEL 1 goto :error

cd ..\tools
%MAKE% install
if ERRORLEVEL 1 goto :error

echo.
pause
exit /b


:error
echo.
echo Compilation Error!!!
echo.
pause
exit /b
