@ECHO OFF
@setlocal

rem
rem Edit this line to run the batch file for Qt environment.
rem

call "C:\Qt\5.12.3\msvc2017_64\bin\qtenv2.bat"
rem call "C:\Qt\5.12.3\msvc2017\bin\qtenv2.bat"
rem call "C:\Qt\5.12.3\msvc2015_64\bin\qtenv2.bat"
rem call "C:\Qt\5.12.3\mingw73_64\bin\qtenv2.bat"

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
rem call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
rem call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64

cd /D %~dp0
call compile_install.bat

pause
exit /b
