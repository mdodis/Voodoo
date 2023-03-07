@echo off

pushd "%~dp0"
set cwd="%cd%"
popd

pushd %cwd%\..\
cmake -DCMAKE_BUILD_TYPE=Release . -B Build-Release
popd