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

for %%I in (nmake.exe) do if exist %%~$path:I set NMAKE=%%~$path:I
if "%NMAKE%" == "" (
  for %%I in (jom.exe) do if exist %%~$path:I set NMAKE=%%~$path:I
  if not "%NMAKE%" == "" (
    set NMAKE=jom
  )
) else (
  set NMAKE=nmake VERBOSE=1
)
for %%I in (qmake.exe) do if exist %%~$path:I set QMAKE=%%~$path:I
for %%I in (cmake.exe) do if exist %%~$path:I set CMAKE=%%~$path:I
for %%I in (sqlite3.exe) do if exist %%~$path:I set SQLITE=%%~$path:I
if "%SQLITE%" == "" for %%I in (sqlite3-bin.exe) do if exist %%~$path:I set SQLITE=%%~$path:I

if "%NMAKE%" == "" (
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
rd /Q /S %APPNAME% >nul 2>nul
tspawn new %APPNAME% --template erb
if "%SQLITE%" == "" (
  echo;
  echo sqlite.exe command not found.
  call :CleanUp
  exit /B 1
)
"%SQLITE%" %DBFILE% < create_blog_table.sql

cd %APPDIR%
tspawn s blog
tspawn w foo

:: Test in debug mode
call :CMakeBuild Debug
if ERRORLEVEL 1 exit /B %ERRORLEVEL%
call :CheckWebApp treefrogd.exe
if ERRORLEVEL 1 exit /B %ERRORLEVEL%

call :QMakeBuild debug
if ERRORLEVEL 1 exit /B %ERRORLEVEL%
call :CheckWebApp treefrogd.exe
if ERRORLEVEL 1 exit /B %ERRORLEVEL%
nmake distclean >nul 2>nul

:: Test in release mode
call :CMakeBuild Release
if ERRORLEVEL 1 exit /B %ERRORLEVEL%
call :CheckWebApp treefrog.exe
if ERRORLEVEL 1 exit /B %ERRORLEVEL%

call :QMakeBuild release
if ERRORLEVEL 1 exit /B %ERRORLEVEL%
call :CheckWebApp treefrog.exe
if ERRORLEVEL 1 exit /B %ERRORLEVEL%
nmake distclean >nul 2>nul

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
"%QMAKE%" -r CONFIG+=%1
%NMAKE%
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

echo "%TREEFROG%" -v
"%TREEFROG%" -v 2>&1
echo "%TREEFROG%" -l
"%TREEFROG%" -l 2>&1
echo "%TREEFROG%" --show-routes
"%TREEFROG%" --show-routes 2>&1
if ERRORLEVEL 1 (
  echo App Error!
  exit /B 1
)
echo;

echo "%TREEFROG%" --settings
"%TREEFROG%" --settings 2>&1
if ERRORLEVEL 1 (
  echo App Error!
  type log\treefrog.log
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
rd /Q /S %APPNAME% >nul 2>nul
exit /B 0

:: which cmd
:Which
for %%I in (%1 %1.com %1.exe %1.bat %1.cmd %1.vbs %1.js %1.wsf) do if exist %%~$path:I SET TFENV=%%~$path:I
exit /B
