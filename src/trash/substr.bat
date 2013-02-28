@echo OFF
:: Prints a part of a string.
::   %1 : input string
::   %2 : start index
::   %3 : length
setlocal

set str=%~1
set idx=%2
set len=%3

if "%idx%" == "" (
  echo %str%
  exit /b
)

if "%len%" == "" (
  call set result=%%str:~%idx%%%
) else (
  call set result=%%str:~%idx%,%len%%%
)

if not "%result%" == "" echo %result%
