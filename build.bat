@ECHO OFF
@setlocal

::
:: Edit this line to run the batch file for Qt environment.
::
::call "C:\Qt\Qt5.2.1\5.2.1\mingw48_32\bin\qtenv2.bat"
::call "C:\Qt\Qt5.3.2-mingw\5.3\mingw482_32\bin\qtenv2.bat"
call "C:\Qt\Qt5.3.2\5.3\msvc2013_64\bin\qtenv2.bat"



if "%Platform%" == "X64" (
  set MAKE=nmake
) else if "%DevEnvDir%" == "" (
  set MAKE=mingw32-make -j4
) else (
  set MAKE=nmake
  set CL=/MP
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
