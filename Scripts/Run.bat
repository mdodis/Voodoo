@echo off

pushd "%~dp0"
set cwd="%cd%"
popd

pushd %cwd%\..
.\Build\Engine\Debug\vkx.exe
popd