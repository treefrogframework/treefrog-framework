@ECHO OFF
@setlocal

::
:: Edit this line to run the batch file for Qt environment.
::
call "C:\Qt\Qt5.1.0\5.1.0\mingw48_32\bin\qtenv2.bat"
::call "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\bin\qtenv2.bat"

::
:: Compile and Install
::
cd /D %~dp0

call configure.bat --enable-mongo --enable-debug
if ERRORLEVEL 1 goto :error

cd src
mingw32-make.exe -j4 install
if ERRORLEVEL 1 goto :error

cd ..\tools
mingw32-make.exe -j4 install
if ERRORLEVEL 1 goto :error

cd ..
call configure.bat --enable-mongo
if ERRORLEVEL 1 goto :error

cd src
mingw32-make.exe -j4 install
if ERRORLEVEL 1 goto :error

cd ..\tools
mingw32-make.exe -j4 install
if ERRORLEVEL 1 goto :error

echo.
pause
exit /b


:error
echo.
echo Compilation Error!
pause
exit /b
