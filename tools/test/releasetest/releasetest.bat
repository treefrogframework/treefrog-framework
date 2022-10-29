@echo off
@setlocal

set BASEDIR=%~dp0
set APPNAME=blogapp
set APPDIR=%BASEDIR%%APPNAME%
set DBFILE=%APPDIR%\db\dbfile
set PORT=8800
set MAKE=nmake VERBOSE=1

cd /D %BASEDIR%
call :Which tfenv.bat
if not "%TFENV%" == "" (
  call "%TFENV%"
) else (
  call "..\..\..\tfenv.bat"
)

for %%I in (nmake.exe) do if exist %%~$path:I set NMAKE=%%~$path:I
for %%I in (qmake.exe) do if exist %%~$path:I set QMAKE=%%~$path:I
for %%I in (cmake.exe) do if exist %%~$path:I set CMAKE=%%~$path:I
for %%I in (sqlite3.exe) do if exist %%~$path:I set SQLITE=%%~$path:I
if "%SQLITE%" == "" for %%I in (sqlite3-bin.exe) do if exist %%~$path:I set SQLITE=%%~$path:I

if "%NMAKE%" == "" (
  echo;
  echo nmake.exe command not found.
  call :CleanUp
  pause
  exit /B 1
)

if "%QMAKE%" == "" (
  echo;
  echo qmake.exe command not found.
  call :CleanUp
  pause
  exit /B 1
)


cd /D %BASEDIR%
rd /Q /S %APPNAME%
tspawn new %APPNAME%
if "%SQLITE%" == "" (
  echo;
  echo sqlite.exe command not found.
  call :CleanUp
  pause
  exit /B 1
)
"%SQLITE%" %DBFILE% < create_blog_table.sql

cd %APPDIR%
echo n | tspawn s blog
tspawn w foo

:: Test in debug mode
if not "%CMAKE%" == "" (
  call :CMakeBuild Debug
  call :CheckWebApp treefrogd
)

call :QMakeBuild debug
call :CheckWebApp treefrogd
%MAKE% distclean >nul 2>nul

:: Test in release mode
if not "%CMAKE%" == "" (
  call :CMakeBuild Release
  call :CheckWebApp treefrog
)

call :QMakeBuild release
call :CheckWebApp treefrog
%MAKE% distclean >nul 2>nul

echo;
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
"%CMAKE%" -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=%1 ..
%MAKE%
if ERRORLEVEL 1 (
  echo;
  echo Build Error!
  call :CleanUp
  exit /B 1
)
exit /B 0

::
:: Build by qmake
::
:QMakeBuild
cd /D %APPDIR%
%QMAKE% -r CONFIG+=%1
%MAKE%
if ERRORLEVEL 1 (
  echo;
  echo Build Error!
  call :CleanUp
  exit /B 1
)
exit /B 0

::
:: Check WebApp
::
:CheckWebApp
cd /D %APPDIR%
"%1" --show-routes
if ERRORLEVEL 1 (
  echo App Error!
  exit /B 1
)
echo;
echo Starting webapp..
set RES=1
"%1" -e dev -d -p %PORT% %APPDIR%
if ERRORLEVEL 1 (
  echo App Start Error!
  exit /B 1
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
  echo;
  echo App Test Error!
  call :CleanUp
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
