# libbsa

Reusable C++ library for reading and writing Bethesda Archive formats.

## Build Setup

Dependencies are managed with vcpkg manifest mode. The current required ports are:

- `libdeflate` for deflate compression and decompression
- `lz4` for the official liblz4 library
- `DirectXTex` for texture analysis in later milestones
- `catch2` for the CTest-discoverable milestone test suite

On this machine vcpkg is installed at `C:\vcpkg`. Configure the default static library build with:

```powershell
$env:VCPKG_ROOT = 'C:\vcpkg'
cmake --fresh --preset vcpkg
cmake --build --preset vcpkg
ctest --test-dir out/build/vcpkg -C Debug --output-on-failure
```

To build `libbsa` as a dynamic library instead, use the shared preset:

```powershell
$env:VCPKG_ROOT = 'C:\vcpkg'
cmake --fresh --preset vcpkg-shared
cmake --build --preset vcpkg-shared
ctest --test-dir out/build/vcpkg-shared -C Debug --output-on-failure
```
