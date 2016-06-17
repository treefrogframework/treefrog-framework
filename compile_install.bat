@ECHO OFF
@setlocal

:: Requires Qt environment.


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
exit /b


:error
echo.
echo Compilation Error!!!
echo.
exit /b
