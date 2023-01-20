
## Requirements

```
vcpkg install tbb cgal
```

## Build (windows)

```
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/source/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release --target ALL_BUILD -- /maxcpucount:14
```