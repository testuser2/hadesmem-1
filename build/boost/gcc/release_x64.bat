set OLDCD=%CD%
pushd %BOOST_ROOT%
set OLDPATH=%PATH%
set PATH=%MINGW64%;%MINGW64%\bin\;%PATH%
b2 -j %NUMBER_OF_PROCESSORS% --with-exception --with-thread --with-date_time --with-program_options toolset=gcc cxxflags=-std=c++11 address-model=64 architecture=x86 variant=release cxxflags=-flto --stagedir=stage/gcc-x64 link=static runtime-link=shared threading=multi debug-symbols=on define=STRICT define=STRICT_TYPED_ITEMIDS define=UNICODE define=_UNICODE define=WINVER=_WIN32_WINNT_VISTA define=_WIN32_WINNT=_WIN32_WINNT_VISTA define=BOOST_USE_WINDOWS_H stage > %OLDCD%\release_x64.txt 2>&1
set PATH=%OLDPATH%
popd
