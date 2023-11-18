#!/bin/bash

libs="-luser32 -lgdi32 -lopengl32"
warnings="-Wno-writable-strings -Wno-format-security -Wno-deprecated-declarations -Wno-switch"
includes="-Ithird_party -Ithird_party/Include"

clang++ $includes -g src/main.cpp -o celesteclone.exe $libs $warnings