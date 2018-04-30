@ECHO OFF
@setlocal

set BASEDIR=%~dp0
set APPNAME=blogapp
set APPDIR=%BASEDIR%\%APPNAME%
set DBFILE=%APPDIR%\db\dbfile
set PORT=8800

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
if ERRORLEVEL 1 (
  echo.
  echo App Test Error!
  call ::CleanUp
  exit /B 1
)

%MAKE% distclean >nul 2>nul

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
if ERRORLEVEL 1 (
  echo.
  echo App Test Error!
  call ::CleanUp
  exit /B 1
)

echo.
echo Test OK
call ::CleanUp
exit /B

::
:: Check WebApp
::
:CheckWebApp
echo Starting webapp..
set RES=1
treefrog -e dev -d -p %PORT% %APPDIR%
if ERRORLEVEL 1 (
  echo App Start Error!
)

timeout 1 /nobreak >nul
set URL=http://localhost:%PORT%/blog
set CMD=curl -s "%URL%" -w "%%{http_code}" -o nul
for /f "usebackq delims=" %%a in (`%CMD%`) do set RESCODE=%%a
if "%RESCODE%"=="200" (
  echo HTTP request success "%URL%"
  set RES=0
) else (
  echo HTTP request failed
)
treefrog -k stop %APPDIR%
exit /B %RES%

::
:: CleanUp Subroutine
::
:CleanUp
  cd /D %BASEDIR%
  rd /Q /S %APPNAME%
exit /B 0
