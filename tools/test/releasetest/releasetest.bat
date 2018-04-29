@ECHO OFF
@setlocal

set BASEDIR=%~dp0
set APPNAME=blogapp
set DBFILE=%APPNAME%\db\dbfile

cd /D %BASEDIR%
call "..\..\..\tfenv.bat"

if "%Platform%" == "X64" (
  set MAKE=nmake
  set CL=/MP
) else if "%DevEnvDir%" == "" (
  set MAKE=mingw32-make -j4
) else (
  set MAKE=nmake
  set CL=/MP
)

cd /D %BASEDIR%
rd /Q /S %APPNAME%
tspawn new %APPNAME%
sqlite3.exe %DBFILE% < create_blog_table.sql

cd %APPNAME%
tspawn s blog
tspawn w foo
qmake -r CONFIG+=release
%MAKE%
if ERRORLEVEL 1 (
  echo.
  echo Build Error!
  call ::CleanUp
  exit /B 1
)

mkdir build
cd build
cmake -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=Release ..
%MAKE%
if ERRORLEVEL 1 (
  echo.
  echo Build Error!
  call ::CleanUp
  exit /B 1
)

echo.
echo Build OK
call ::CleanUp
exit /B


::
:: CleanUp Subroutine
::
:CleanUp
  cd /D %BASEDIR%
  rd /Q /S %APPNAME%
exit /B 0
