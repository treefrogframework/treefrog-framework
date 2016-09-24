@ECHO OFF
@setlocal

::
:: Edit this line to run the batch file for Qt environment.
::
::call "C:\Qt\Qt5.5.1-MinGW\5.5\mingw492_32\bin\qtenv2.bat"
::call "C:\Qt\Qt5.6.0-MinGW\5.4\mingw492_32\bin\qtenv2.bat"
::call "C:\Qt\Qt5.5.1\5.5\msvc2013_64\bin\qtenv2.bat"
::call "C:\Qt\Qt5.6.0\5.6\msvc2013_64\bin\qtenv2.bat"
call "C:\Qt\Qt5.7.0\5.7\msvc2015_64\bin\qtenv2.bat"

::call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" amd64
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\Tools\VsDevCmd.bat"

cd /D %~dp0
call compile_install.bat

pause
exit /b