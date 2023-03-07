@echo off

pushd "%~dp0"
set cwd="%cd%"
popd

pushd %cwd%\..\
cmake --build Build-Release --config Release
popd