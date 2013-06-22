@ECHO OFF
@setlocal
::
:: 
::
set TFDIR=C:\TreeFrog\1.6.1
set PATH="C:\Program Files (x86)\WiX Toolset v3.7\bin";%PATH%


%~d0
cd  %~dp0


::mklink /D  SourceDir %TFDIR%

:: Creates Fragment file
heat.exe dir %TFDIR% -dr INSTALLDIR -cg TreeFrogFiles -gg -out TreeFrogFiles.wxs

:: Creates installer
candle.exe TreeFrog.wxs TreeFrogFiles.wxs 
light.exe -out TreeFrog-1.x.x.msi TreeFrog.wixobj TreeFrogFiles.wixobj

echo.
pause


