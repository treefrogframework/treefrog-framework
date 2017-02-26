@ECHO OFF
@setlocal
::
:: Edit this line to run the batch file for Qt environment.
::

set VERSION=1.15.0
set TFDIR=C:\TreeFrog\%VERSION%
set BASEDIR=%~dp0
set SLNFILE=%BASEDIR%\treefrog-setup\treefrog-setup.sln
cd %BASEDIR%

:: MinGW
::call :build_msi "C:\Qt\Qt5.6.2-mingw\5.6\mingw49_32\bin\qtenv2.bat"   5.6
::call :build_msi "C:\Qt\Qt5.7.1-mingw\5.7\mingw53_32\bin\qtenv2.bat"   5.7
::call :build_setup treefrog-%VERSION%-mingw-setup.exe


:: MSVC2013
::call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" amd64
::call :build_msi "C:\Qt\Qt5.6.1-msvc2013\5.6\msvc2013_64\bin\qtenv2.bat"  5.6
::call :build_msi "C:\Qt\Qt5.7.1-msvc2013\5.7\msvc2013_64\bin\qtenv2.bat"  5.7
::call :build_setup treefrog-%VERSION%-msvc2013_64-setup.exe


:: MSVC2015
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64
call :build_msi "C:\Qt\Qt5.6.2-msvc2015\5.6\msvc2015_64\bin\qtenv2.bat"  5.6
call :build_msi "C:\Qt\Qt5.7.1-msvc2015\5.7\msvc2015_64\bin\qtenv2.bat"  5.7
call :build_setup treefrog-%VERSION%-msvc2015_64-setup.exe


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
set PATH="C:\Program Files (x86)\WiX Toolset v3.10\bin";%PATH%
set MSINAME=TreeFrog-SDK-Qt%1.msi

cd /D msi

@rd SourceDir 2>nul
mklink /D  SourceDir %TFDIR%
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
"C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\IDE\devenv" %SLNFILE%  /rebuild release
if ERRORLEVEL 1 goto :error
move %BASEDIR%\treefrog-setup\Release\treefrog-setup.exe %BASEDIR%\treefrog-setup\Release\%1
goto :eof


:error
echo.
echo Bat Error!!!
echo.
pause
exit /b
