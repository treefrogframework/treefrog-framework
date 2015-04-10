@ECHO OFF
@setlocal

::call "C:\TreeFrog\1.7.9\bin\tfenv.bat"

set APPNAME=blogapp
set DBFILE=%APPNAME%\db\dbfile


if "%Platform%" == "X64" (
  set MAKE=nmake
  set CL=/MP
) else if "%DevEnvDir%" == "" (
  set MAKE=mingw32-make -j4
) else (
  set MAKE=nmake
  set CL=/MP
)

cd /D %~dp0
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

echo.
echo Build OK
call ::CleanUp
exit /B


::
:: CleanUp Subroutine
::
:CleanUp
  del /F /Q /S *.* >NUL
  cd ..
  rd /Q /S %APPNAME%
exit /B 0
