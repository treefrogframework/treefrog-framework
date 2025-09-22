REM Supported/used environment variables:
REM   LINK_STATIC              Whether to statically link to libmongoc
REM   ENABLE_SSL               Enable SSL with Microsoft Secure Channel
REM   ENABLE_SNAPPY            Enable Snappy compression

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

if "%ENABLE_SNAPPY%"=="1" (
  rem Enable Snappy
  curl -sS --retry 5 -LO https://github.com/google/snappy/archive/1.1.7.tar.gz || goto :error
  %TAR% xzf 1.1.7.tar.gz || goto :error
  cd snappy-1.1.7 || goto :error
  %CMAKE% -G "Visual Studio 15 2017" -A x64 -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR% -S snappy-1.1.7 -B snappy-1.1.7-build || goto :error
  %CMAKE% --build snappy-1.1.7-build --config "Debug" --target ALL_BUILD -- /m || goto :error
  %CMAKE% --build snappy-1.1.7-build --config "Debug" --target INSTALL -- /m || goto :error
  set SNAPPY_OPTION=-DENABLE_SNAPPY=ON
) else (
  set SNAPPY_OPTION=-DENABLE_SNAPPY=OFF
)

cd %BUILD_DIR% || goto :error
rem Build libmongoc
if "%ENABLE_SSL%"=="1" (
  %CMAKE% -G "Visual Studio 15 2017" -A x64 -DCMAKE_PREFIX_PATH=%INSTALL_DIR%\lib\cmake -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR% -DENABLE_SSL=WINDOWS %ENABLE_SNAPPY_OPTION% .. || goto :error
) else (
  %CMAKE% -G "Visual Studio 15 2017" -A x64 -DCMAKE_PREFIX_PATH=%INSTALL_DIR%\lib\cmake -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR% -DENABLE_SSL=OFF %ENABLE_SNAPPY_OPTION% .. || goto :error
)

%CMAKE% --build . --config "Debug" --target ALL_BUILD -- /m || goto :error
%CMAKE% --build . --config "Debug" --target INSTALL -- /m || goto :error

rem Test our CMake package config file with CMake's find_package command.
set EXAMPLE_DIR=%SRCROOT%\src\libmongoc\examples\cmake\find_package

if "%LINK_STATIC%"=="1" (
  set EXAMPLE_DIR="%EXAMPLE_DIR%_static"
)

cd %EXAMPLE_DIR% || goto :error

if "%ENABLE_SSL%"=="1" (
  cp ..\..\..\tests\x509gen\client.pem . || goto :error
  cp ..\..\..\tests\x509gen\ca.pem . || goto :error
  set MONGODB_EXAMPLE_URI="mongodb://localhost/?ssl=true&sslclientcertificatekeyfile=client.pem&sslcertificateauthorityfile=ca.pem&sslallowinvalidhostnames=true"
)

%CMAKE% -G "Visual Studio 15 2017" -A x64 -DCMAKE_PREFIX_PATH=%INSTALL_DIR%\lib\cmake . || goto :error
%CMAKE% --build . --config "Debug" --target ALL_BUILD -- /m || goto :error

rem Yes, they should've named it "dependencies".
dumpbin.exe /dependents Debug\hello_mongoc.exe || goto :error

Debug\hello_mongoc.exe %MONGODB_EXAMPLE_URI% || goto :error

goto :EOF
:error
exit /B %errorlevel%
