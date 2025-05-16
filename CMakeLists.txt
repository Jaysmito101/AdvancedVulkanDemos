cmake_minimum_required(VERSION 3.12)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(ps C CXX)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

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

set(ASSETS_TIMESTAMP_FILE ${CMAKE_SOURCE_DIR}/avd_assets/generated/.asset_timestamp)

file(GLOB_RECURSE ASSET_SOURCE_FILES
    ${CMAKE_SOURCE_DIR}/avd_assets/*
    ${CMAKE_SOURCE_DIR}/assets/*
)

file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/avd_assets/generated)

add_custom_command(
    OUTPUT ${ASSETS_TIMESTAMP_FILE}
    COMMAND ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/assets.py
    COMMAND ${CMAKE_COMMAND} -E touch ${ASSETS_TIMESTAMP_FILE}
    COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR} # Regenerate build files
    DEPENDS ${ASSET_SOURCE_FILES} ${CMAKE_SOURCE_DIR}/tools/assets.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating assets and regenerating build files..."
)

add_custom_target(generate_assets ALL DEPENDS ${ASSETS_TIMESTAMP_FILE})

add_subdirectory(dep/glfw)
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