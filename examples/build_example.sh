#!/bin/bash

# Build the Permuto API example
echo "Building Permuto API example..."

# Ensure main library is built
if [ ! -f "../build/libpermuto.a" ]; then
    echo "Building main library first..."
    cd ..
    cmake -B build -DCMAKE_BUILD_TYPE=Release -DPERMUTO_BUILD_TESTS=OFF
    cmake --build build
    cd examples
fi

# Compile the example
g++ -std=c++17 \
    -I ../include \
    -I /usr/include/nlohmann \
    api_example.cpp \
    -L ../build \
    -lpermuto \
    -o api_example

if [ $? -eq 0 ]; then
    echo "✓ Build successful! Run with: ./api_example"
else
    echo "✗ Build failed!"
    exit 1
fi