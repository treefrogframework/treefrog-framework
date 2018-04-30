@ECHO OFF
@setlocal

set BASEDIR=%~dp0
set APPNAME=blogapp
set APPDIR=%BASEDIR%\%APPNAME%
set DBFILE=%APPDIR%\db\dbfile

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
call ::CheckWebApp
%MAKE% distclean >nul

cd /D %APPDIR%
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
call ::CheckWebApp

echo.
echo Test OK
call ::CleanUp
exit /B

::
:: Check WebApp
::
:CheckWebApp
cd /D %APPDIR%
echo Starting webapp..
treefrog -e dev -d
if ERRORLEVEL 1 (
  echo.
  echo App Start Error!
  call ::CleanUp
  exit /B 1
)
timeout 3 /nobreak >nul
treefrog -k abort
exit /B 0

::
:: CleanUp Subroutine
::
:CleanUp
  cd /D %BASEDIR%
  rd /Q /S %APPNAME%
exit /B 0
