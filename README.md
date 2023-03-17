# VKX

A simplistic ECS engine, with just one Vulkan renderer, primarily focused on experimentation and ease of use.

## Platforms

| Platform        | Supported |
| --------------- | --------- |
| Windows | Yes |
| Linux | No (Planned) |
| Mac | No |

## Development Requirements

- Vulkan SDK
- Visual Studio C/C++ Build Tools (2022 or later)
- CMake

## Building

All third party libraries are included in source, so you only have to:
1. Run `Scripts/0.1 - Configure.bat`
2. Run `Scripts/0.0 - Build.bat`
3. Run `Scripts/0.3 - Build Shaders.bat`

## Third Party Libraries

- [flecs](https://www.flecs.dev/flecs/)
- [glm](https://github.com/g-truc/glm)
- [ImGui](https://github.com/ocornut/imgui)
- [LZ4](https://github.com/lz4/lz4)
- [STB](https://github.com/nothings/stb)
- [Vulkan Memory Allocator](https://gpuopen.com/vulkan-memory-allocator/)
- [Tinyobjloader](https://github.com/tinyobjloader/tinyobjloader)