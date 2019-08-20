VTKMovieRenderer
================

# Summary
- [Description](#description)
- [Compilation](#compilation-requirements)
- [Install](#install)
- [Screenshots](#screenshots)
- [Repository information](#repository-information)

# Description
Small helper application to generate videos using the VTK and ITK libraries. All the scripting and animation is done in C++, so the application must be compiled every time the scene changes. 

# Compilation requirements
## To build the tool:
* cross-platform build system: [CMake](http://www.cmake.org/cmake/resources/software.html).
* compiler: [Mingw64](http://sourceforge.net/projects/mingw-w64/) on Windows.

## External dependencies
The following libraries are required:
* [Qt 5 opensource framework](http://www.qt.io/).
* [ITK](https://itk.org/) - Insight Segmentation and Registration Toolkit.
* [VTK](http://www.vtk.org/) - The Visualization Toolkit.

# Install
The only current option is build from source as binaries are not provided. 

# Screenshots
Main dialog allows the user to reposition the camera in the view before the rendering process and configure a minimal set of rendering options. 

![maindialog](https://user-images.githubusercontent.com/12167134/48957944-5791c200-ef5c-11e8-8445-3bd70f2347d0.jpg)

# Repository information

**Version**: 1.1.0

**Status**: finished

**cloc statistics**

| Language                     |files          |blank        |comment           |code  |
|:-----------------------------|--------------:|------------:|-----------------:|-----:|
| C++                          |   5           | 338         |   183            | 1279 |
| C/C++ Header                 |   4           | 103         |   253            |  202 |
| CMake                        |   1           |  17         |     3            |   58 |
| **Total**                    | **10**        | **458**     | **439**          | **1539** |
