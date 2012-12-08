@echo OFF
::
:: TreeFrog Application Startup Batch
::
setlocal
title TreeFrog Application Startup
set PATH=C:\TreeFrog\0.85.0\bin;C:\Qt\4.8.1\bin;%PATH%

::
:: Run command: treefrog or treefrogd
::   treefrog   : Run in release mode
::   treefrogd  : Run in debug mode
::
set CMD=treefrogd

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
