cmake_minimum_required (VERSION 2.8.6)

if(CMAKE_MAJOR_VERSION EQUAL 3 OR CMAKE_MAJOR_VERSION GREATER 3)
  cmake_policy(SET CMP0048 OLD) # Don't use project version
endif()

project (VTKMovieRenderer)

add_definitions(
  -std=c++14
  -fpermissive)
  
if(DEFINED MINGW)
	add_definitions(-mwindows -m64)
else(DEFINED MINGW)
	add_definitions(-rdynamic)
endif(DEFINED MINGW)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Find the QtWidgets library
find_package(Qt5 COMPONENTS Widgets Multimedia)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Multimedia_EXECUTABLE_COMPILE_FLAGS}")

find_package (VTK REQUIRED NO_MODULE)
include (${VTK_USE_FILE})

find_package (ITK REQUIRED)
include (${ITK_USE_FILE})

set (VTK_MOVIE_RENDERER_VERSION_MAJOR 1)
set (VTK_MOVIE_RENDERER_VERSION_MINOR 1)
set (VTK_MOVIE_RENDERER_VERSION_PATCH 0)

# Use the include path and library for Qt that is used by VTK.
INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(RCC_FILES
  rsc/resources.qrc
  )

qt5_wrap_ui (UI_FILES
  MovieRenderer.ui
  )
  
set (SOURCE_FILES
  main.cpp
  MovieRenderer.cpp
  ResourceLoader.cpp
  ScriptExecutor.cpp
  Utils.cpp
  )

#Set any libraries that your project depends on.
SET(Libraries
  Qt5::Widgets
  Qt5::Multimedia
  ${ITK_LIBRARIES}
  ${VTK_LIBRARIES}
  ${QT_LIBRARIES}
)

if(DEFINED MINGW OR DEFINED MSVC)
  set (Libraries ${Libraries} ${QT_QTTEST_LIBRARY})
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/MovieRenderer.rc.in" "${CMAKE_CURRENT_BINARY_DIR}/MovieRenderer.rc")
  SET(SOURCE_FILES ${SOURCE_FILES} ${CMAKE_CURRENT_BINARY_DIR}/MovieRenderer.rc)
  SET(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff -o <OBJECT> -i <SOURCE>")
  ENABLE_LANGUAGE(RC)
endif (DEFINED MINGW OR DEFINED MSVC)

ADD_EXECUTABLE(VTKMovieRenderer ${SOURCE_FILES} ${MOC_FILES} ${UI_FILES} ${RCC_FILES})
TARGET_LINK_LIBRARIES(VTKMovieRenderer ${Libraries})
