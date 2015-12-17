@ECHO OFF
@setlocal

::
:: Edit this line to run the batch file for Qt environment.
::
::call "C:\Qt\Qt5.4.2-MinGW\5.4\mingw491_32\bin\qtenv2.bat"
::call "C:\Qt\Qt5.5.1-MinGW\5.5\mingw492_32\bin\qtenv2.bat"
::call "C:\Qt\Qt5.4.0-msvc2013_64\5.4\msvc2013_64_opengl\bin\qtenv2.bat"
::call "C:\Qt\Qt5.5.1\5.5\msvc2013_64\bin\qtenv2.bat"

::call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" amd64

cd /D %~dp0
call compile_install.bat

pause
exit /b