@ECHO OFF
@setlocal

::
:: Edit this line to run the batch file for Qt environment.
::
::call "C:\Qt\Qt5.6.2-mingw\5.6\mingw49_32\bin\qtenv2.bat"
::call "C:\Qt\Qt5.7.1-mingw\5.7\mingw53_32\bin\qtenv2.bat"
::call "C:\Qt\Qt5.8.0-msvc2015\5.8\msvc2015_64\bin\qtenv2.bat"
call "C:\Qt\Qt5.9.1\5.9.1\msvc2015_64\bin\qtenv2.bat"

::call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" amd64
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64

cd /D %~dp0
call compile_install.bat

pause
exit /b
