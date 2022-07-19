# Need CMake >= 3.0 because of use of CMake targets like ROOT::Libraries etc.
cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

# Name of this project
project(panguin LANGUAGES CXX)

# Install in GNU-style directory layout  (copied from japan/CMakeLists.txt)
include(GNUInstallDirs)


message(STATUS "System name ${CMAKE_SYSTEM_NAME}")
#MAC specific variable
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_MACOSX_RPATH ON)
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
    if("${isSystemDir}" STREQUAL "-1")
        set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
    endif()
endif()

# Local path for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

#  Install directory
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  message(STATUS "    Install-prefix was at default -> forcing it to the source-dir" )
  message(STATUS "    Use -DCMAKE_INSTALL_PREFIX=/usr/local to set to something else" )
  set (CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}"
        CACHE PATH "default install path" FORCE )
else()
  set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR})
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
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

# ROOT dictionary needed for interactive ROOT graphics support
build_root_dictionary(
    panguin             # dictionary base name -> panguinDict.cxx
    ${PROJECT_SOURCE_DIR}/include/panguinOnlineConfig.hh
    ${PROJECT_SOURCE_DIR}/include/panguinOnline.hh
    TARGETS panguin
    LINKDEF "${PROJECT_SOURCE_DIR}/include/panguinOnlineLinkDef.h"  # ROOT linkDef file
    OPTIONS -p
)


include_directories(${PROJECT_SOURCE_DIR}/include)


#----------------------------------------------------------------------------
# Locate sources and headers for this project
# NB: headers are included so they will show up in IDEs
#
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

#----------------------------------------------------------------------------
# Add the executable
#
add_library(panguin-lib SHARED ${sources} ${headers} panguinDict.cxx)
set_target_properties(panguin-lib PROPERTIES OUTPUT_NAME panguin)
target_link_libraries(panguin-lib PUBLIC ${PODD_LIBS} ROOT::Libraries)

add_executable(panguin-bin panguin.cc)
set_target_properties(panguin-bin PROPERTIES OUTPUT_NAME panguin)
target_link_libraries(panguin-bin panguin-lib)

#----------------------------------------------------------------------------
#
add_custom_target(panguin DEPENDS panguin-bin)

#----------------------------------------------------------------------------
# Install the executable to 'bin' directory under CMAKE_INSTALL_PREFIX
#
#install(TARGETS panguin-bin DESTINATION ${CMAKE_INSTALL_BINDIR})
#install(TARGETS panguin-lib DESTINATION ${CMAKE_INSTALL_LIBDIR})
