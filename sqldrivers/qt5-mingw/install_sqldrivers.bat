@ECHO OFF
@setlocal

set BASEDIR=%~dp0
cd /D "%BASEDIR%"

@rem check tspawn.exe command 
for %%I in (tspawn.exe) do if exist %%~$path:I (
  goto :COPY
)

if exist ..\bin\tfenv.bat (
  call ..\bin\tfenv.bat
) else (
  if exist ..\..\tfenv.bat call ..\..\tfenv.bat
)
for %%I in (tspawn.exe) do if exist %%~$path:I (
  goto :COPY
)

echo command: tspawn not found.
echo Execute this batch file in TreeFrog command prompt.
exit /B


:COPY
for /f "usebackq tokens=4,5" %%V in (`qmake.exe -v`) do @set VERSION=%%V

for /f "usebackq tokens=*" %%J in (`tspawn.exe --show-driver-path`) do @set DRIVERPATH=%%J

if not "%VERSION%" == "" if exist "%VERSION%" (
  if not "%DRIVERPATH%" == "" if exist "%DRIVERPATH%" (
    cd %VERSION%
    echo copy to %DRIVERPATH% ..
    copy /Y /B *.dll "%DRIVERPATH%"
    exit /B
  )
)

echo error: plugins directory not found.
