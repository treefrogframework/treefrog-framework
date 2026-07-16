rem Load environment for Visual Studio 17 2022.
rem https://learn.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-170
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" || goto :error

echo on
echo

rem Use DENABLE_SSL=OFF. Windows hosts do not have a MinGW ABI compatible OpenSSL install.
set CMAKE_FLAGS=-DENABLE_SSL=OFF -DENABLE_SASL=SSPI
set TAR=C:\cygwin\bin\tar

set SRCROOT=%CD%
set BUILD_DIR=%CD%\build-dir
rmdir /S /Q %BUILD_DIR% 2>nul || true
mkdir %BUILD_DIR% || goto :error

set INSTALL_DIR=%CD%\install-dir
rmdir /S /Q %INSTALL_DIR% 2>nul || true
mkdir %INSTALL_DIR% || goto :error

set PATH=%PATH%;%INSTALL_DIR%\bin

set major=1

cd %BUILD_DIR% || goto :error

rem Build libmongoc, with flags that the downstream R driver mongolite uses
uvx cmake -G "Ninja" -DMONGO_USE_LLD=OFF -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR% -DCMAKE_C_FLAGS="-pedantic" -DCMAKE_PREFIX_PATH=%INSTALL_DIR%\lib\cmake %CMAKE_FLAGS% .. || goto :error
uvx cmake --build . --parallel || goto :error
uvx cmake --build . --target install || goto :error

rem Test our pkg-config file
set EXAMPLE_DIR=%SRCROOT%\src\libmongoc\examples\
cd %EXAMPLE_DIR% || goto :error

rem Proceed from here once we have pkg-config on Windows
exit /B 0

set PKG_CONFIG_PATH=%INSTALL_DIR%\lib\pkgconfig

rem http://stackoverflow.com/questions/2323292
for /f %%i in ('pkg-config --libs --cflags mongoc%major%') do set PKG_CONFIG_OUT=%%i

echo PKG_CONFIG_OUT is %PKG_CONFIG_OUT%

%CC% -o hello_mongoc hello_mongoc.c %PKG_CONFIG_OUT% || goto :error

dumpbin.exe /dependents Debug\hello_mongoc.exe || goto :error

rem Add DLLs to PATH
set PATH=%PATH%;%INSTALL_DIR%\bin

Debug\hello_mongoc.exe %MONGODB_EXAMPLE_URI% || goto :error

goto :EOF
:error
exit /B %errorlevel%
