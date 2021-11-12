@ECHO OFF
@setlocal
::
:: Edit this line to run the batch file for Qt environment.
::

:: 10�s�ځA21�s�ځA22�s�ڂ�ҏW


set VERSION=2.2.0
set QTBASE=C:\Qt
set TFDIR=C:\TreeFrog\%VERSION%

set BASEDIR=%~dp0
set SLNFILE=%BASEDIR%\treefrog-setup\treefrog-setup.sln
cd %BASEDIR%


:: Clear environment variables
set VCToolsVersion=
set VSINSTALLDIR=
set VisualStudioVersion=
set INCLUDE=
set LIB=
set PATH=C:\WINDOWS\system32;C:\WINDOWS

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
call :build_msi "%QTBASE%\6.2.0\msvc2019_64\bin\qtenv2.bat"      6.2

:: Clear environment variables
set VCToolsVersion=
set VSINSTALLDIR=
set VisualStudioVersion=
set INCLUDE=
set LIB=
set PATH=C:\WINDOWS\system32;C:\WINDOWS

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
call :build_msi "%QTBASE%\6.1.2\msvc2019_64\bin\qtenv2.bat"      6.1

call :build_setup treefrog-%VERSION%-msvc_64-setup.exe

echo.
echo.
echo Creating setup files ... Completed
pause
exit /B


:::�T�u���[�`��:::

::===�r���h���s 
:build_msi
@setlocal
if not exist %1 (
  echo File not found %1
  pause
  exit /B
)
call %1
if exist "%TFDIR%" rmdir /s /q "%TFDIR%"
cd /D %BASEDIR%
call ..\compile_install.bat
call :create_installer %2
goto :eof


::===�C���X�g�[��(msi)�쐬
:create_installer
@setlocal
set PATH="C:\Program Files (x86)\WiX Toolset v3.11\bin";%PATH%
set MSINAME=TreeFrog-SDK-Qt%1.msi

cd /D msi

rd /s /q  SourceDir >nul 2>&1
del /f /q SourceDir >nul 2>&1
mklink /j SourceDir %TFDIR%
if ERRORLEVEL 1 goto :error

:: Creates Fragment file
heat.exe dir %TFDIR% -dr INSTALLDIR -cg TreeFrogFiles -gg -out TreeFrogFiles.wxs
if ERRORLEVEL 1 goto :error

:: Creates installer
candle.exe TreeFrog.wxs TreeFrogFiles.wxs
if ERRORLEVEL 1 goto :error
light.exe  -ext WixUIExtension -out %MSINAME% TreeFrog.wixobj TreeFrogFiles.wixobj
if ERRORLEVEL 1 goto :error

rd SourceDir 
echo.
echo ----------------------------------------------------
echo Created installer   [ %TFDIR% ]  --^>  %MSINAME%
echo.
goto :eof


::===�Z�b�g�A�b�vEXE�쐬
:build_setup
@setlocal
"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\devenv" %SLNFILE%  /rebuild release
if ERRORLEVEL 1 goto :error
move %BASEDIR%\treefrog-setup\Release\treefrog-setup.exe %BASEDIR%\treefrog-setup\Release\%1
goto :eof


:error
echo.
echo Bat Error!!!
echo.
pause
exit /b
