cmake_minimum_required(VERSION 3.12)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(avd C CXX)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SHADERC_SKIP_TESTS ON)

# Use FindVulkan module added with CMAKE 3.7
if(NOT CMAKE_VERSION VERSION_LESS 3.7.0)
  message(STATUS "Using module to find Vulkan")
  find_package(Vulkan)
endif()

IF(UNIX AND NOT APPLE)
  set(LINUX TRUE)
ENDIF()

IF(WIN32)
  IF(NOT Vulkan_FOUND)
    find_library(Vulkan_LIBRARY NAMES vulkan-1 vulkan PATHS ${CMAKE_SOURCE_DIR}/libs/vulkan)

    IF(Vulkan_LIBRARY)
      set(Vulkan_FOUND ON)
      MESSAGE("Using bundled Vulkan library version")
    ENDIF()
  ENDIF()

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
ELSEIF(LINUX)
  IF(NOT Vulkan_FOUND)
    find_library(Vulkan_LIBRARY NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "${CMAKE_SOURCE_DIR}/libs/vulkan" REQUIRED)

    IF(Vulkan_LIBRARY)
      set(Vulkan_FOUND ON)
      MESSAGE("Using bundled Vulkan library version")
    ENDIF()
  ENDIF()
ENDIF()

IF(NOT Vulkan_FOUND)
  message(FATAL_ERROR "Could not find Vulkan library!")
ELSE()
  message(STATUS ${Vulkan_LIBRARY})
ENDIF()


find_package(Python COMPONENTS Interpreter REQUIRED)

file(GLOB_RECURSE ASSET_SOURCE_FILES
    ${CMAKE_SOURCE_DIR}/avd_assets/*.h
    ${CMAKE_SOURCE_DIR}/avd_assets/*.c
    ${CMAKE_SOURCE_DIR}/assets/*
)


if (WIN32)
  set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
elseif (APPLE)
  set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_MACOS_MVK)
else()
  set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_XLIB_KHR)
endif()

set(PA_BUILD_SHARED_LIBS OFF)

if(CMAKE_C_COMPILER_ID STREQUAL "Clang" AND WIN32)
  set(PA_USE_WDMKS OFF CACHE BOOL "Disable WDMKS for Clang" FORCE)
  set(PA_USE_WDMKS_DEVICE_INFO OFF CACHE BOOL "Disable WDMKS device info for Clang" FORCE)
  set(PA_USE_WASAPI OFF CACHE BOOL "Disable WASAPI for Clang" FORCE)
endif()

add_subdirectory(dep/glfw)
add_subdirectory(dep/volk)
add_subdirectory(dep/portaudio)
add_subdirectory(avd_assets)
add_subdirectory(avd)

if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.12.25835)
  set(CMAKE_CXX20_STANDARD_COMPILE_OPTION "-std:c++latest")
  set(CMAKE_CXX20_EXTENSION_COMPILE_OPTION "-std:c++latest")
endif()

if( MSVC )
  if(${CMAKE_VERSION} VERSION_LESS "3.6.0")
    message( "\n\t[ WARNING ]\n\n\tCMake version lower than 3.6.\n\n\t - Please update CMake and rerun; OR\n\t - Manually set 'GLFW-CMake-starter' as StartUp Project in Visual Studio.\n" )
  else()
    set_property( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT avd )
  endif()
endif()
