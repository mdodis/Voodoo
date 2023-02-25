@echo off
set cwd=%~dp0

set compiler=%VULKAN_SDK%\Bin\glslc.exe

pushd %cwd%\..\Shaders

%compiler% .\Basic.vert -o Basic.vert.spv
%compiler% .\Basic.frag -o Basic.frag.spv

popd