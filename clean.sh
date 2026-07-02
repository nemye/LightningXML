#!/bin/bash

# Clean script to remove build and bin directories
cd $(dirname "$0")

# Remove build directory
if [ -d "build" ]; then
    echo "Removing build directory..."
    rm -r build
fi

if [ -d "install" ]; then
    echo "Removing install directory..."
    rm -r install
fi

# Remove bin directory
if [ -d "bin" ]; then
    echo "Removing bin directory..."
    rm -r bin
fi

echo "Clean completed."