rem Load environment for Visual Studio 15 2017.
rem https://learn.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-150
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvars64.bat" || goto :error

echo on
echo

set TAR=C:\cygwin\bin\tar
set LINK_STATIC=1

set SRCROOT=%CD%
set BUILD_DIR=%CD%\build-dir
rmdir /S /Q %BUILD_DIR% 2>nul || true
mkdir %BUILD_DIR% || goto :error

set INSTALL_DIR=%CD%\install-dir
rmdir /S /Q %INSTALL_DIR% 2>nul || true
mkdir %INSTALL_DIR% || goto :error

set PATH=%PATH%;%INSTALL_DIR%\bin

set major=1
set version=1.31.0

cd %BUILD_DIR% || goto :error

rem Build libmongoc, with flags that the downstream R driver mongolite uses
%CMAKE% -G "Ninja" DCMAKE_INSTALL_PREFIX=%INSTALL_DIR% -DCMAKE_C_FLAGS="-pedantic" -DCMAKE_PREFIX_PATH=%INSTALL_DIR%\lib\cmake -DENABLE_STATIC=ON .. || goto :error
%CMAKE% --build . --parallel || goto :error
%CMAKE% --build . --target install || goto :error

rem Test our pkg-config file
set EXAMPLE_DIR=%SRCROOT%\src\libbson\examples\
cd %EXAMPLE_DIR% || goto :error

rem Proceed from here once we have pkg-config on Windows
exit /B 0

set PKG_CONFIG_PATH=%INSTALL_DIR%\lib\pkgconfig

rem http://stackoverflow.com/questions/2323292
for /f %%i in ('pkg-config --libs --cflags bson%major%') do set PKG_CONFIG_OUT=%%i

echo PKG_CONFIG_OUT is %PKG_CONFIG_OUT%

%CC% -o hello_bson hello_bson.c %PKG_CONFIG_OUT% || goto :error

dumpbin.exe /dependents Debug\hello_bson.exe || goto :error

rem Add DLLs to PATH
set PATH=%PATH%;%INSTALL_DIR%\bin

Debug\hello_bson.exe %MONGODB_EXAMPLE_URI% || goto :error

goto :EOF
:error
exit /B %errorlevel%
