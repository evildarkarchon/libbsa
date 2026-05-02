# libbsa

Reusable C++ library for reading and writing Bethesda Archive formats.

## Build Setup

Dependencies are managed with vcpkg manifest mode. The current required ports are:

- `libdeflate` for deflate compression and decompression
- `lz4` for the official liblz4 library

On this machine vcpkg is installed at `C:\vcpkg`. Configure with:

```powershell
$env:VCPKG_ROOT = 'C:\vcpkg'
cmake --preset vcpkg
cmake --build --preset vcpkg
```
