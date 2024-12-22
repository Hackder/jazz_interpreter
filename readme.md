# Compilation
```
cmake -S . -B build
cd build
cmake --build . -j 10
```

Two binaries will be generated, one test binary `jazz_test` which will run
the unit tests, and the other `jazz` which is the main program.
