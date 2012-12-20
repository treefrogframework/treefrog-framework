@echo OFF
::
:: TreeFrog Application Startup Batch
::

setlocal
title TreeFrog Application Startup
call C:\TreeFrog\1.1.1\bin\tfenv.bat >nul 2>&1

::
:: Run command: treefrog or treefrogd
::   treefrog.exe   : Run in release mode
::   treefrogd.exe  : Run in debug mode
::
set CMD=treefrogd.exe

::
:: Database environment: product, test, dev, etc.
::
set ENV=dev


echo.
echo Database Environment: %ENV%
echo TreeFrog application servers are starting up ...

%CMD% -e %ENV% %~dp0

if not errorlevel 1 goto finish

:error
echo.
pause

:finish
