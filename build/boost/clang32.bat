set OLDCD=%CD%
pushd %BOOST_ROOT%
set OLDPATH=%PATH%
set PATH=%MINGW32_CLANG%;%MINGW32_CLANG%\bin\;%PATH%
b2 -j %NUMBER_OF_PROCESSORS% --without-python toolset=clang cxxflags=-std=c++11 address-model=32 architecture=x86 --stagedir=stage/clang-x86 link=static runtime-link=shared threading=multi debug-symbols=on define=STRICT define=STRICT_TYPED_ITEMIDS define=UNICODE define=_UNICODE define=WINVER=_WIN32_WINNT_VISTA define=_WIN32_WINNT=_WIN32_WINNT_VISTA define=BOOST_USE_WINDOWS_H define=BOOST_FILESYSTEM_NO_DEPRECATED define=BOOST_SYSTEM_NO_DEPRECATED stage > %OLDCD%\clang32.txt 2>&1
set PATH=%OLDPATH%
popd
