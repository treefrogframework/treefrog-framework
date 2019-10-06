@ECHO OFF
@setlocal

rem
rem Edit this line to run the batch file for Qt environment.
rem

set BASEDIR=%~dp0

call "C:\Qt\5.13.0\msvc2017_64\bin\qtenv2.bat"
rem call "C:\Qt\5.13.0\msvc2017\bin\qtenv2.bat"
rem call "C:\Qt\5.13.0\msvc2015_64\bin\qtenv2.bat"
rem call "C:\Qt\5.12.4\msvc2017_64\bin\qtenv2.bat"
rem call "C:\Qt\5.12.4\msvc2017\bin\qtenv2.bat"
rem call "C:\Qt\5.12.4\msvc2015_64\bin\qtenv2.bat"


set VSVER=2017
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
  echo %VCVARSBAT%  %ARCH%
  call %VCVARSBAT% %ARCH%
) else (
  call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" %ARCH%
  rem call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" %ARCH%
)

cd /D %BASEDIR%
call compile_install.bat

pause
exit /b
