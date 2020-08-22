@ECHO OFF
@setlocal

call "..\..\tfenv.bat"

set MAKE=nmake
set CL=/MP

cd /D %~dp0
if exist Makefile (
  %MAKE% distclean
)
qmake -r CONFIG+=debug
%MAKE%
if ERRORLEVEL 1 goto :build_error

for /d %%d in (*) do (
  set TESTNAME=%%d
  echo ---------------------------------------------------------------------
  if exist %%d\debug\%%d.exe (
    echo Testing %%d\debug\%%d.exe ...

    cd %%d
    debug\%%d.exe
    if ERRORLEVEL 1 goto :error
    cd ..
  ) else if exist %%d\%%d.bat (
    echo Testing %%d\%%d.bat ...

    cd %%d
    call %%d.bat
    if ERRORLEVEL 1 goto :error
    cd ..
  )
)


echo.
echo All tests passed. Congratulations!
echo.
pause
exit /b


:build_error
echo.
echo Build error!  [ %TESTNAME% ]
echo.
pause
exit /b

:error
echo.
echo Execute error!  [ %TESTNAME% ]
echo.
pause
exit /b
