@ECHO OFF
@setlocal
::
:: Batch File for Creating Installer 
::  - Run as an administrator
::  - Requires WiX Toolset
::

set VERSION=1.12.0
set TFDIR=C:\TreeFrog\%VERSION%
set PATH="C:\Program Files (x86)\WiX Toolset v3.10\bin";%PATH%

cd /D %~dp0

mklink /D  SourceDir %TFDIR%

:: Creates Fragment file
heat.exe dir %TFDIR% -dr INSTALLDIR -cg TreeFrogFiles -gg -out TreeFrogFiles.wxs

:: Creates installer
candle.exe TreeFrog.wxs TreeFrogFiles.wxs 
light.exe  -ext WixUIExtension -out TreeFrog-SDK.msi TreeFrog.wixobj TreeFrogFiles.wixobj

rd SourceDir 
echo.
echo ----------------------------------------------------
echo Created installer   [ %TFDIR% ]  --^>  TreeFrog-SDK.msi
echo.
pause
