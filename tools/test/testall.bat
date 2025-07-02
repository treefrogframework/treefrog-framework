@ECHO OFF
@setlocal

call "..\..\tfenv.bat"

set MAKE=nmake
set CL=/MP

cd /D %~dp0
if exist Makefile (
  %MAKE% distclean
)

for /d %%d in (*) do (
  set TESTNAME=%%d
  echo ---------------------------------------------------------------------

  cd %%d
  if exist Makefile (
    %MAKE% distclean
  )
  if exist %%d.pro (
    qmake CONFIG+=debug
    %MAKE%
    if ERRORLEVEL 1 goto :build_error
  )
  cd ..

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
exit /b


:build_error
echo.
echo Build error!  [ %TESTNAME% ]
echo.
exit /b

:error
echo.
echo Execute error!  [ %TESTNAME% ]
echo.
exit /b
