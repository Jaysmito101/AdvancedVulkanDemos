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

add_subdirectory(dep/glfw)
add_subdirectory(ps_assets)

find_package(Python COMPONENTS Interpreter REQUIRED)

set(ASSETS_TIMESTAMP_FILE ${CMAKE_SOURCE_DIR}/ps_assets/generated/.asset_timestamp)

file(GLOB_RECURSE ASSET_SOURCE_FILES
    ${CMAKE_SOURCE_DIR}/ps_assets/*
    ${CMAKE_SOURCE_DIR}/assets/*
)

file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/ps_assets/generated)

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

include_directories(
    ./ps
    ./ps/include
    ./dep/glfw/include
    ./dep/stb
    ./dep/cute_headers
    ./ps_assets/generated/include
    ${Vulkan_INCLUDE_DIRS}
)

file(GLOB_RECURSE ps_headers
    ./ps/*.h
)


add_executable(ps
    ${ps_headers}
    ./ps/src/ps_main.c
    ./ps/src/ps_application.c
    ./ps/src/ps_window.c
    ./ps/src/ps_vulkan.c
    ./ps/src/ps_vulkan_swapchain.c
    ./ps/src/ps_vulkan_renderer.c
    ./ps/src/ps_vulkan_presentation.c
    ./ps/src/ps_vulkan_framebuffer.c
    ./ps/src/ps_vulkan_image.c
    ./ps/src/ps_vulkan_buffer.c
    ./ps/src/ps_shader_compiler.c
    ./ps/src/ps_utils.c
    ./ps/src/ps_shader.c
    ./ps/src/ps_input.c
    ./ps/src/ps_third_party_impls.c
    ./ps/src/ps_pipeline_utils.c
    ./ps/src/ps_font_renderer.c
    ./ps/src/ps_bloom.c

    ./ps/src/scenes/ps_scenes.c
    ./ps/src/scenes/ps_scenes_splash.c
    ./ps/src/scenes/ps_scenes_loading.c
    ./ps/src/scenes/ps_scenes_mainmenu.c
    ./ps/src/scenes/ps_scenes_prologue.c
)
target_link_libraries(ps
    ps_assets
    glfw
    ${Vulkan_LIBRARY}
    debug $ENV{VULKAN_SDK}/Lib/shaderc_combinedd.lib
    optimized $ENV{VULKAN_SDK}/Lib/shaderc_combined.lib
)

# Make the main target depend on the assets
add_dependencies(ps generate_assets)

target_compile_definitions(ps PRIVATE
    $<$<CONFIG:Debug>:PS_DEBUG>          # Define PS_DEBUG for Debug builds
    $<$<NOT:$<CONFIG:Debug>>:PS_RELEASE> # Define PS_RELEASE for non-Debug builds
)


if (WIN32)
    target_compile_definitions(ps
        PUBLIC _CRT_SECURE_NO_WARNINGS
    )
endif()

if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.12.25835)
    set(CMAKE_CXX20_STANDARD_COMPILE_OPTION "-std:c++latest")
    set(CMAKE_CXX20_EXTENSION_COMPILE_OPTION "-std:c++latest")
endif()

if( MSVC )
    if(${CMAKE_VERSION} VERSION_LESS "3.6.0") 
        message( "\n\t[ WARNING ]\n\n\tCMake version lower than 3.6.\n\n\t - Please update CMake and rerun; OR\n\t - Manually set 'GLFW-CMake-starter' as StartUp Project in Visual Studio.\n" )
    else()
        set_property( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ps )
    endif()

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ignore:4099")
endif()