@ECHO OFF
@setlocal

set BASEDIR=%~dp0
set APPNAME=blogapp
set APPDIR=%BASEDIR%\%APPNAME%
set DBFILE=%APPDIR%\db\dbfile
set PORT=8800

cd /D %BASEDIR%
call :Which tfenv.bat
if not "%TFENV%" == "" (
  call "%TFENV%"
) else (
  call "..\..\..\tfenv.bat"
)
if "%Platform%" == "X64" (
  set MAKE=nmake VERBOSE=1
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

:: Test in debug mode
call :CMakeBuild Debug
call :CheckWebApp treefrogd

call :QMakeBuild debug
call :CheckWebApp treefrogd
%MAKE% distclean >nul 2>nul

:: Test in release mode
call :CMakeBuild Release
call :CheckWebApp treefrog
%MAKE% clean  >nul 2>nul

call :QMakeBuild release
call :CheckWebApp treefrog
%MAKE% distclean >nul 2>nul

echo.
echo Test OK
call :CleanUp
exit /B 0


::
:: Build by cmake
::
:CMakeBuild
cd /D %APPDIR%
if exist build rd /Q /S build
mkdir build >nul 2>nul
cd build
cmake -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=%1 ..
%MAKE%
if ERRORLEVEL 1 (
  echo.
  echo Build Error!
  call ::CleanUp
  exit/ B 1
)
exit /B 0


::
:: Build by qmake
::
:QMakeBuild
cd /D %APPDIR%
qmake -r CONFIG+=%1
%MAKE%
if ERRORLEVEL 1 (
  echo.
  echo Build Error!
  call ::CleanUp
  exit /B 1
)
exit /B 0

::
:: Check WebApp
::
:CheckWebApp
cd /D %APPDIR%
echo.
echo Starting webapp..
set RES=1
"%1" -e dev -d -p %PORT% %APPDIR%
if ERRORLEVEL 1 (
  echo App Start Error!
)

timeout 1 /nobreak >nul
set URL=http://localhost:%PORT%/blog
set CMD=curl -s "%URL%" -w "%%{http_code}" -o nul
set RESCODE=0
for /f "usebackq delims=" %%a in (`%CMD%`) do set RESCODE=%%a
"%1" -k stop %APPDIR%
if ERRORLEVEL 1 (
  "%1" -k abort %APPDIR%
)
timeout 1 /nobreak >nul
if not "%RESCODE%"=="200" (
  echo HTTP request failed
  echo.
  echo App Test Error!
  call ::CleanUp
  exit /B 1
)
echo HTTP request success "%URL%"
exit /B 0

::
:: CleanUp Subroutine
::
:CleanUp
cd /D %BASEDIR%
rd /Q /S %APPNAME%
exit /B 0

:: which cmd
:Which
for %%I in (%1 %1.com %1.exe %1.bat %1.cmd %1.vbs %1.js %1.wsf) do if exist %%~$path:I SET TFENV=%%~$path:I
exit /B
