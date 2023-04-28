@echo off

pushd "%~dp0"
set cwd="%cd%"
popd

pushd %cwd%\..
.\Build\Source\Editor\Debug\Editor.exe
popd