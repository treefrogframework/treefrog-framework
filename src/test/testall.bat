@ECHO OFF
@setlocal

::
:: Edit this line to run the batch file for Qt environment.
::
call "C:\TreeFrog\1.7.9\bin\tfenv.bat"


if "%DevEnvDir%" == "" (
  set MAKE=mingw32-make -j4
) else (
  set MAKE=nmake
  set CL=/MP
)


cd /D %~dp0
if exist Makefile (
  %MAKE% distclean
)
qmake -r CONFIG+=release
%MAKE%
if ERRORLEVEL 1 goto :build_error

for /d %%d in (*) do (
  set TESTNAME=%%d
  echo ---------------------------------------------------------------------
  if exist %%d\release\%%d.exe (
    echo Testing %%d\release\%%d.exe ...

    cd %%d
    release\%%d.exe
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
