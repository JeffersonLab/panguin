# Need CMake >= 3.0 because of use of CMake targets like ROOT::Libraries etc.
cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

# Name of this project
project(panguin LANGUAGES CXX VERSION 2.8)

# Install in GNU-style directory layout  (copied from japan/CMakeLists.txt)
include(GNUInstallDirs)

message(STATUS "System name ${CMAKE_SYSTEM_NAME}")
# Local path for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

#  Install directory
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  message(STATUS "    Install-prefix was at default -> forcing it to the source-dir" )
  message(STATUS "    Use -DCMAKE_INSTALL_PREFIX=/usr/local to set to something else" )
  set (CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}"
        CACHE PATH "default install path" FORCE )
endif()

# RPATH handling
if(UNIX)
  set(CMAKE_SKIP_BUILD_RPATH FALSE)
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
  list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_FULL_LIBDIR}" isSystemDir)
  # Only set RPATH if not installing into a system directory
  if(isSystemDir STREQUAL "-1")
    if(NOT CMAKE_SYSTEM_NAME STREQUAL Darwin)
      # Linux etc.
      set(CMAKE_INSTALL_RPATH "\$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
    else()
      # macOS
      set(CMAKE_MACOSX_RPATH ON)
      set(CMAKE_INSTALL_RPATH "@loader_path/../${CMAKE_INSTALL_LIBDIR}")
    endif()
  endif()
endif()

# Load ROOT and setup include directory
find_package(ROOT 6 REQUIRED Gui Minuit2)
include_directories(${ROOT_INCLUDE_DIR})

# If available, link with Hall A analyzer libraries to avoid nuisance warnings
# about missing dictionaries (THaRun, THaEventHeader, etc.) in ROOT files
if(DEFINED ENV{ANALYZER})
  set(CMAKE_PREFIX_PATH "$ENV{ANALYZER}" "${CMAKE_PREFIX_PATH}")
  find_package(Podd 1.6 QUIET)
  if(Podd_FOUND)
    set(PODD_LIBS Podd::Podd)
  endif()
endif()

# CLI11 command line parser
file(DOWNLOAD
  "https://github.com/CLIUtils/CLI11/releases/download/v2.3.2/CLI11.hpp"
  "${CMAKE_BINARY_DIR}/CLI11.hpp"
  EXPECTED_HASH
    SHA256=ba83806399a66634ca8f8d292df031e5ed651315ceb9a6a09ba56f88d75f1797
  STATUS _ret
)
list(GET _ret 0 _retcode)
if(NOT _ret EQUAL 0)
  list(GET _ret 1 _errtxt)
  message(FATAL_ERROR "Failed to download CLI11.hpp: ${_errtxt}")
endif()
include_directories(${CMAKE_BINARY_DIR})

# ROOT dictionary needed for interactive ROOT graphics support
build_root_dictionary(
    panguin             # dictionary base name -> panguinDict.cxx
    "${PROJECT_SOURCE_DIR}/include/panguinOnlineConfig.hh"
    "${PROJECT_SOURCE_DIR}/include/panguinOnline.hh"
    TARGETS panguin
    LINKDEF "${PROJECT_SOURCE_DIR}/include/panguinOnlineLinkDef.h"  # ROOT linkDef file
    OPTIONS -p
)

#----------------------------------------------------------------------------
# Locate sources and headers for this project
# NB: headers are included so they will show up in IDEs
#
include_directories(${PROJECT_SOURCE_DIR}/include)
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

#----------------------------------------------------------------------------
# Add the executable
#
add_library(panguin-lib SHARED ${sources} ${headers} panguinDict.cxx)
set_target_properties(panguin-lib PROPERTIES OUTPUT_NAME panguin)
target_link_libraries(panguin-lib PUBLIC ${PODD_LIBS} ROOT::Libraries)

add_executable(panguin-bin panguin.cc "${CMAKE_BINARY_DIR}/CLI11.hpp")
set_target_properties(panguin-bin PROPERTIES OUTPUT_NAME panguin)
target_link_libraries(panguin-bin panguin-lib)

#----------------------------------------------------------------------------
#
add_custom_target(panguin DEPENDS panguin-bin)

#----------------------------------------------------------------------------
# Install the executable to 'bin' directory under CMAKE_INSTALL_PREFIX
#
install(TARGETS panguin-bin DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS panguin-lib DESTINATION ${CMAKE_INSTALL_LIBDIR})
