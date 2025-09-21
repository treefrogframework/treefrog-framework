REM Supported/used environment variables:
REM   LINK_STATIC              Whether to statically link to libbson

rem Ensure Cygwin executables like sh.exe are not in PATH
rem set PATH=C:\Windows\system32;C:\Windows

rem Load environment for Visual Studio 15 2017.
rem https://learn.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-150
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvars64.bat" || goto :error

echo on
echo

set TAR=C:\cygwin\bin\tar

set SRCROOT=%CD%
set BUILD_DIR=%CD%\build-dir
rmdir /S /Q %BUILD_DIR% 2>nul || true
mkdir %BUILD_DIR% || goto :error

set INSTALL_DIR=%CD%\install-dir
rmdir /S /Q %INSTALL_DIR% 2>nul || true
mkdir %INSTALL_DIR% || goto :error

set PATH=%PATH%;%INSTALL_DIR%\bin

cd %BUILD_DIR% || goto :error

if "%LINK_STATIC%"=="1" (
  %CMAKE% -G "Visual Studio 15 2017" -A x64 -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR% -DENABLE_TESTS=OFF .. || goto :error
) else (
  %CMAKE% -G "Visual Studio 15 2017" -A x64 -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR% -DENABLE_TESTS=OFF -DENABLE_STATIC=OFF .. || goto :error
)

%CMAKE% --build . --config "Debug" --target ALL_BUILD -- /m || goto :error
%CMAKE% --build . --config "Debug" --target INSTALL -- /m || goto :error

rem Test our CMake package config file with CMake's find_package command.
set EXAMPLE_DIR=%SRCROOT%\src\libbson\examples\cmake\find_package

if "%LINK_STATIC%"=="1" (
  set EXAMPLE_DIR="%EXAMPLE_DIR%_static"
)

cd %EXAMPLE_DIR% || goto :error
%CMAKE% -G "Visual Studio 15 2017" -A x64 -DCMAKE_PREFIX_PATH=%INSTALL_DIR%\lib\cmake . || goto :error
%CMAKE% --build . --config "Debug" --target ALL_BUILD -- /m || goto :error

rem Yes, they should've named it "dependencies".
dumpbin.exe /dependents Debug\hello_bson.exe || goto :error

Debug\hello_bson.exe || goto :error

goto :EOF
:error
exit /B %errorlevel%
