@ECHO OFF
@setlocal

set BASEDIR=%~dp0
cd %BASEDIR%drivers

@rem check tspawn.exe command 
for %%I in (tspawn.exe) do if exist %%~$path:I (
  goto :COPY
)
echo command: tspawn not found.
echo Execute this batch file in TreeFrog command prompt.
pause
exit /B


:COPY
for /f "usebackq tokens=*" %%J in (`tspawn --show-driver-path`) do @set DRIVERPATH=%%J
if not "%DRIVERPATH%" == "" if exist "%DRIVERPATH%" (
  echo copy to %DRIVERPATH% ..
  copy /Y /B *.dll "%DRIVERPATH%"
  exit /B
)

echo error: plugins directory not found.
