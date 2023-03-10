@echo off

pushd "%~dp0"
set cwd="%cd%"
popd

pushd %cwd%\..
.\Build\MokLib\Tests\Debug\MokLibTests.exe
popd