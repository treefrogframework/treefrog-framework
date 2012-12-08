@echo OFF
::
:: TreeFrog Application Abort Batch
::
setlocal
set PATH=C:\TreeFrog\0.85.0\bin;C:\Qt\4.8.1\bin;%PATH%

::
:: Run command: treefrog or treefrogd
::
set CMD=treefrogd


echo.
%CMD% -k abort %~dp0

if not errorlevel 1 goto finish

:error
echo.
pause

:finish
