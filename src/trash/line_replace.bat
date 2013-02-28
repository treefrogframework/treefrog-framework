@echo OFF
::
:: Line Replacement Batch
::   arg1 : file name
::   arg2 : search string
::   arg3 : replacement string
::
setlocal
goto main

:: Calculates the length of the string
::   %1 : string
::   %errorlevel% : length of the string
:strlen
  setlocal
  set s=%~1
  set len=0
  :loop_strlen
  if defined s (
    set s=%s:~1%
    set /A len+=1
    goto :loop_strlen
  )
  exit /B %len%


:: Find the position of the first occurrence of a substring in a string
::   %1 : a character
::   %2 : string to search in
::   %errorlevel% : the position
:strpos
  setlocal
  set c=%1
  set str=%~2
  call :strlen "%str%"
  set loop_max=%errorlevel%
  
  set i=0
  :loop_strpos
  call set ch=%%str:~%i%,1%%
  if %c% == %ch% exit /B %i%
  
  set /A i+=1
  if %i% LSS %loop_max% goto :loop_strpos
  exit /B -1


:: Replaces lines that matched the string
::   %1  : output file
::   %L% : line of string
::   %S% : search string
::   %R% : replacement string
:replace_line
  setlocal
  echo %L% | findstr /R /C:"%S%" > nul
  if not errorlevel 1 (
    echo %R%>> %1
  ) else (
    echo %L%>> %1
  )
  exit /B


:process_line
  setlocal
  set str=%~1
  call :strpos : "%str%"
  set index=%errorlevel%
  set /A index+=1
  
  :: execute substr.bat
  for /F "usebackq tokens=* delims=" %%s in (`call substr "%str%" %index%`) do set L=%%s
  if "%L%" == "" (
    echo.>> %ORIG%
  ) else (
    call :replace_line %ORIG%
  )
  exit /B


::
:: script main
::
:main
if "%1"=="" exit /B
set ORIG=%1
set TMP=%1._dmy_

findstr /N "^" %ORIG% > %TMP%
type nul > %ORIG%
    
set S=%~2
set R=%~3
    
for /f "tokens=* delims=" %%i in (%TMP%) do (
  call :process_line "%%i"
)
del %TMP% > nul

:end
