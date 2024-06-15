echo on
echo

set TAR=C:\cygwin\bin\tar
set CMAKE_MAKE_PROGRAM=C:\mingw-w64\x86_64-4.9.1-posix-seh-rt_v3-rev1\mingw64\bin\mingw32-make.exe
set CC=C:\mingw-w64\x86_64-4.9.1-posix-seh-rt_v3-rev1\mingw64\bin\gcc.exe
rem Ensure Cygwin executables like sh.exe are not in PATH
set "PATH=C:\cygwin\bin;C:\Windows\system32;C:\Windows;C:\mingw-w64\x86_64-4.9.1-posix-seh-rt_v3-rev1\mingw64\bin;C:\mongoc;src\libbson;src\libmongoc;C:\Program Files\Git\cmd"
set LINK_STATIC=1

set SRCROOT=%CD%
set BUILD_DIR=%CD%\build-dir
rmdir /S /Q %BUILD_DIR%
mkdir %BUILD_DIR%

set INSTALL_DIR=%CD%\install-dir
rmdir /S /Q %INSTALL_DIR%
mkdir %INSTALL_DIR%

set PATH=%PATH%;%INSTALL_DIR%\bin

cd %BUILD_DIR%
robocopy "%SRCROOT%" "%BUILD_DIR%" /E /XD ".git" "%BUILD_DIR%" "_build" "cmake-build" /NP /NFL /NDL

rem Build libmongoc, with flags that the downstream R driver mongolite uses
%CMAKE% -G "MinGW Makefiles" -DCMAKE_MAKE_PROGRAM=%CMAKE_MAKE_PROGRAM% -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR% -DCMAKE_CFLAGS="-pedantic" -DCMAKE_PREFIX_PATH=%INSTALL_DIR%\lib\cmake -DENABLE_STATIC=ON .
%CMAKE% --build . --parallel
if errorlevel 1 (
   exit /B 1
)

%CMAKE% --build . --target install
if errorlevel 1 (
   exit /B 1
)

set MINGW=1
call ..\.evergreen\scripts\check-installed-files-bson.bat
if errorlevel 1 (
   exit /B 1
)
set MINGW=

if not exist %INSTALL_DIR%\lib\libbson-static-1.0.a (
  echo libbson-static-1.0.a missing!
  exit /B 1
) else (
  echo libbson-static-1.0.a check ok
)

cd %SRCROOT%

rem Test our pkg-config file
set EXAMPLE_DIR=%SRCROOT%\src\libbson\examples\
cd %EXAMPLE_DIR%

rem Proceed from here once we have pkg-config on Windows
exit /B 0

set PKG_CONFIG_PATH=%INSTALL_DIR%\lib\pkgconfig

rem http://stackoverflow.com/questions/2323292
for /f %%i in ('pkg-config --libs --cflags libbson-1.0') do set PKG_CONFIG_OUT=%%i

echo PKG_CONFIG_OUT is %PKG_CONFIG_OUT%

%CC% -o hello_bson hello_bson.c %PKG_CONFIG_OUT%

rem Works on windows-64-vs2013-compile, VS 2013 is a.k.a. "Visual Studio 12"
rem And yes, they should've named the flag "dependencies".
"c:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin\dumpbin.exe" /dependents Debug\hello_bson.exe

rem Add DLLs to PATH
set PATH=%PATH%;%INSTALL_DIR%\bin

Debug\hello_bson.exe %MONGODB_EXAMPLE_URI%
