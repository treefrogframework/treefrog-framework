@echo OFF
::
:: TreeFrog Application Shutdown Batch
::
setlocal
set PATH=C:\TreeFrog\0.85.0\bin;C:\Qt\4.8.1\bin;%PATH%

::
:: Run command: treefrog or treefrogd
::
set CMD=treefrogd


echo.
%CMD% -k stop %~dp0

if not errorlevel 1 goto finish

:error
echo.
pause

:finish
