rem "make install" would work, except we run tests on different Evergreen hosts,
rem in different working directories, than where the driver was built. This
rem causes errors in "make install" like:
rem CMake Error at cmake_install cannot find
rem "C:/data/mci/d3ba3391950aca9119e841818a8884bc/mongoc/src/libbson/libbson-1.0.dll"
rem
rem So just copy files manually

rmdir /Q /S C:\mongo-c-driver
md C:\mongo-c-driver
md C:\mongo-c-driver\bin
copy /Y libmongoc-1.0.dll C:\mongo-c-driver\bin
copy /Y cmake-build\src\libbson\libbson-1.0.dll C:\mongo-c-driver\bin

.\cmake-build\src\libmongoc\test-libmongoc.exe --no-fork -d -F test-results.json --skip-tests .evergreen\etc\skip-tests.txt
