@ECHO OFF
@setlocal

::
:: Edit this line to run the batch file for Qt environment.
::
::call "C:\Qt\5.9.3\mingw53_32\bin\qtenv2.bat"
::call "C:\Qt\5.9.3\msvc2015_64\bin\qtenv2.bat"
::call "C:\Qt\5.9.3\msvc2017_64\bin\qtenv2.bat"
::call "C:\Qt\5.10.0\mingw53_32\bin\qtenv2.bat"
::call "C:\Qt\5.10.0\msvc2015_64\bin\qtenv2.bat"
call "C:\Qt\5.11.0\msvc2017_64\bin\qtenv2.bat"
::call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64

cd /D %~dp0
call compile_install.bat

pause
exit /b
