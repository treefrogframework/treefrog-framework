@echo OFF
setlocal
::
:: TreeFrog server startup batch file for Windows service (Debug Mode)
:: 

call C:\TreeFrog\1.1.1\bin\tfenv.bat >nul 2>&1
treefrogd.exe -w %~dp0 >nul 2>&1
