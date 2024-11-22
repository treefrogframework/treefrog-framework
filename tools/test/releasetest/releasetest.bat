@echo off
@setlocal

set BASEDIR=%~dp0
set APPNAME=blogapp
set APPDIR=%BASEDIR%%APPNAME%
set DBFILE=%APPDIR%\db\dbfile
set PORT=18800
set CL=/MP

cd /D %BASEDIR%
call :Which tfenv.bat
if not "%TFENV%" == "" (
  call "%TFENV%"
) else (
  call "..\..\..\tfenv.bat"
)

for %%I in (nmake.exe) do if exist %%~$path:I set MAKE=%%~$path:I
if "%MAKE%" == "" (
  for %%I in (jom.exe) do if exist %%~$path:I set MAKE=%%~$path:I
  if not "%MAKE%" == "" (
    set MAKE=jom
  )
) else (
  set MAKE=nmake VERBOSE=1
)
for %%I in (qmake.exe) do if exist %%~$path:I set QMAKE=%%~$path:I
for %%I in (cmake.exe) do if exist %%~$path:I set CMAKE=%%~$path:I
for %%I in (sqlite3.exe) do if exist %%~$path:I set SQLITE=%%~$path:I
if "%SQLITE%" == "" for %%I in (sqlite3-bin.exe) do if exist %%~$path:I set SQLITE=%%~$path:I

if "%MAKE%" == "" (
  echo;
  echo nmake.exe not found.
  call :CleanUp
  exit /B 1
)

if "%QMAKE%" == "" (
  echo;
  echo qmake.exe command not found.
  call :CleanUp
  exit /B 1
)

if "%CMAKE%" == "" (
  echo;
  echo cmake.exe command not found.
  call :CleanUp
  exit /B 1
)

:: cmake options
if /i "%Platform%" == "x64" (
  set CMAKEOPT=-A x64
) else (
  set CMAKEOPT=-A Win32
)

cd /D %BASEDIR%
rd /Q /S %APPNAME%
tspawn new %APPNAME%
if "%SQLITE%" == "" (
  echo;
  echo sqlite.exe command not found.
  call :CleanUp
  exit /B 1
)
"%SQLITE%" %DBFILE% < create_blog_table.sql

cd %APPDIR%
echo n | tspawn s blog
tspawn w foo

:: Set ExecutionPolicy
@REM for %%I in (tadpoled.exe) do if exist %%~$path:I set TADPOLED=%%~$path:I
@REM for %%I in (tadpole.exe) do if exist %%~$path:I set TADPOLE=%%~$path:I
powershell -Command "Set-ExecutionPolicy -ExecutionPolicy Bypass -Scope CurrentUser -Force"
@REM powershell -command "New-NetFirewallRule -DisplayName MyAppAccess1 -Direction Inbound -Action Allow -Profile Public,Private -Program '%TADPOLED%' -Protocol TCP -LocalPort %PORT% -RemoteAddress 127.0.0.1" >nul 2>&1
@REM powershell -command "New-NetFirewallRule -DisplayName MyAppAccess2 -Direction Inbound -Action Allow -Profile Public,Private -Program '%TADPOLE%' -Protocol TCP -LocalPort %PORT% -RemoteAddress 127.0.0.1" >nul 2>&1

:: Test in debug mode
if not "%CMAKE%" == "" (
  call :CMakeBuild Debug
  if ERRORLEVEL 1 exit /B %ERRORLEVEL%
  call :CheckWebApp treefrogd.exe
  if ERRORLEVEL 1 exit /B %ERRORLEVEL%
)

call :QMakeBuild debug
if ERRORLEVEL 1 exit /B %ERRORLEVEL%
call :CheckWebApp treefrogd.exe
if ERRORLEVEL 1 exit /B %ERRORLEVEL%
%MAKE% distclean >nul 2>nul

:: Test in release mode
if not "%CMAKE%" == "" (
  call :CMakeBuild Release
  if ERRORLEVEL 1 exit /B %ERRORLEVEL%
  call :CheckWebApp treefrog.exe
  if ERRORLEVEL 1 exit /B %ERRORLEVEL%
)

call :QMakeBuild release
if ERRORLEVEL 1 exit /B %ERRORLEVEL%
call :CheckWebApp treefrog.exe
if ERRORLEVEL 1 exit /B %ERRORLEVEL%
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
del /Q /F lib\*.*
cmake --version
set CMD=cmake %CMAKEOPT% -S . -B build -DCMAKE_BUILD_TYPE=%1
echo %CMD%
%CMD%
if ERRORLEVEL 1 (
  echo;
  echo CMake Error!
  call :CleanUp
  exit /B 1
)
cmake --build build --config %1 --clean-first
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
del /Q /F lib\*.*
qmake -r CONFIG+=%1
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

for %%I in (%1) do if exist %%~$path:I set TREEFROG=%%~$path:I
if "%TREEFROG%" == "" (
  echo %1 command not found!
  exit /B 1
)

"%1" -v
"%1" -l
"%1" --show-routes
if ERRORLEVEL 1 (
  echo App Error!
  exit /B 1
)
echo;

"%1" --settings
if ERRORLEVEL 1 (
  echo App Error!
  type log\treefrog.log
  exit /B 1
)
echo;

@REM echo Starting webapp..
@REM set RES=1
@REM "%1" -e dev -d -p %PORT% %APPDIR%
@REM if ERRORLEVEL 1 (
@REM   echo App Start Error!
@REM   exit /B 1
@REM )

@REM timeout 1 /nobreak >nul
@REM set URL=http://localhost:%PORT%/blog
@REM set CMD=curl -s "%URL%" -w "%%{http_code}" -o nul
@REM set RESCODE=0
@REM for /f "usebackq delims=" %%a in (`%CMD%`) do set RESCODE=%%a
@REM "%1" -k stop %APPDIR%
@REM if ERRORLEVEL 1 (
@REM   "%1" -k abort %APPDIR%
@REM )
@REM timeout 1 /nobreak >nul
@REM if not "%RESCODE%"=="200" (
@REM   echo HTTP request failed
@REM   echo;
@REM   echo App Test Error!
@REM   call :CleanUp
@REM   exit /B 1
@REM )
@REM echo HTTP request success "%URL%"

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
