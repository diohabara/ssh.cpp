# scc

C++ command line tool for managing SSH connections.

## Prerequisites

- Docker
- Linux or macOS (Windows is not supported)

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Running

The executable will be created in the `build` directory.

```bash
./build/scc
```

## Testing

To run the tests, use the following command:

```bash
cmake -S . -B build && cmake --build build --parallel && ctest --test-dir build -V
```
