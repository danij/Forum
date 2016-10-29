# Fast Forum Backend

Backend software for providing a discussion forum. 

## Goals

* Fast response times
* Memory efficiency
* Avoiding unnecessary code
* Cross-platform

## Coding Style

* 4 SPACES for indent
* JAVA style but with brackets on new line for C++
* Max 120 characters/line
* Space inside an initializer list (e.g. { 1, 2, 3 } instead of {1, 2, 3})

## Building

    mkdir build
    cd build
    cmake ../
    make

## Running Tests

    ctest --verbose
