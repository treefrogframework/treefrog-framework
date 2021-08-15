@ECHO OFF
@setlocal

rem
rem Edit this line to run the batch file for Qt environment.
rem

set BASEDIR=%~dp0

call "C:\Qt\6.1.1\msvc2019_64\bin\qtenv2.bat"
rem call "C:\Qt\5.13.0\msvc2017\bin\qtenv2.bat"

set VSVER=2019
set ARCH=amd64
set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set VCVARSBAT=""

if exist %VSWHERE% (
  for /f "usebackq tokens=*" %%i in (`%VSWHERE% -find **\vcvarsall.bat`) do (
    echo %%i | find "%VSVER%" >NUL
    if not ERRORLEVEL 1 (
      set VCVARSBAT="%%i"
      goto :break
    )
  )
)
:break

if exist %VCVARSBAT% (
  echo %VCVARSBAT% %ARCH%
  call %VCVARSBAT% %ARCH%
) else (
  echo Error!  Compiler not found.
  pause
  exit /b
)

cd /D %BASEDIR%
call compile_install.bat

pause
exit /b
