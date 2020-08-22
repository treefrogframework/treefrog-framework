@ECHO OFF
@setlocal

cd /D %~dp0

:: Requires Qt environment.

set MAKE=nmake
set CL=/MP

::
:: Compile and Install
::
cd /D %~dp0

call configure.bat --enable-debug
if ERRORLEVEL 1 goto :error

cd src
%MAKE% clean
%MAKE% install
if ERRORLEVEL 1 goto :error

cd ..\tools
%MAKE% clean
%MAKE% install
if ERRORLEVEL 1 goto :error

cd ..
call configure.bat
if ERRORLEVEL 1 goto :error

cd src
%MAKE% clean
%MAKE% install
if ERRORLEVEL 1 goto :error

cd ..\tools
%MAKE% clean
%MAKE% install
if ERRORLEVEL 1 goto :error

echo.
exit /b


:error
echo.
echo Compilation Error!!!
echo.
exit /b
