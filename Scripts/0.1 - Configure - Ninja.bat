@echo off

pushd "%~dp0"
set cwd="%cd%"
popd

pushd %cwd%\..\
cmake . -DCMAKE_BUILD_TYPE=Debug -B Build_Debug -G Ninja
cmake . -DCMAKE_BUILD_TYPE=Release -B Build_Release -G Ninja
popd