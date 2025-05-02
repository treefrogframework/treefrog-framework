@echo off
@setlocal

rem
rem Edit this line to run the batch file for Qt environment.
rem

set BASEDIR=%~dp0

call "C:\Qt\6.9.0\msvc2022_64\bin\qtenv2.bat"
rem call "C:\Qt\6.5.3\msvc2019_64\bin\qtenv2.bat"

set ARCH=amd64
set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set VCVARSBAT=""
set VSVER=2022 2019
set PATH=C:\Qt\Tools\CMake_64\bin;%PATH%

if exist %VSWHERE% (
  for %%v in (%VSVER%) do (
    for /f "usebackq tokens=*" %%i in (`%VSWHERE% -find **\vcvarsall.bat`) do (
      echo %%i | find "%%v" >NUL
      if not ERRORLEVEL 1 (
        set VCVARSBAT="%%i"
        goto :break
      )
    )
  )
)
:break

if exist %VCVARSBAT% (
  echo %VCVARSBAT% %ARCH%
  call %VCVARSBAT% %ARCH%
) else (
  echo Error! Visual Studio not found.
  pause
  exit
)

cd /D %BASEDIR%
call compile_install.bat

pause
exit /b
