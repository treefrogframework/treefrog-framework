@ECHO OFF
@setlocal

::
:: Edit this line to run the batch file for Qt environment.
::
call "C:\Qt\Qt5.0.2\5.0.2\mingw47_32\bin\qtenv2.bat"
::call "C:\Qt\4.8.4\bin\qtvars.bat"

::
:: Compile and Install
::
%~d1 
cd  %~dp0

call configure --enable-mongo --enable-debug
cd src
mingw32-make.exe -j3 install

cd ..\tools
mingw32-make.exe -j3 install

cd ..
call configure --enable-mongo
cd src
mingw32-make.exe -j3 install

cd ..\tools
mingw32-make.exe -j3 install

echo.
pause
