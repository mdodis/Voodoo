@echo off

pushd "%~dp0"
set cwd="%cd%"
popd

pushd %cwd%\..
set CTEST_OUTPUT_ON_FAILURE=1
cmake --build .\Build --target RUN_TESTS
popd