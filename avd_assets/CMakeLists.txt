# glob all the headers(hpp) inside the ps directory
file(GLOB_RECURSE avd_assets_headers
    ./generated/src/*.h
)

file(GLOB_RECURSE avd_assets_sources
    ./generated/src/*.c
)

# if no source files are found, throw an error
if (NOT avd_assets_sources)
    message(FATAL_ERROR "No source files found in the avd_assets directory! Maybe try running the asset generation script.")
endif()


add_library(avd_assets STATIC
    ${avd_assets_headers}
    ${avd_assets_sources}
)

target_include_directories(avd_assets
    PRIVATE ./generated/include
    PRIVATE ../avd/include
    PRIVATE ../dep/glfw/include
)

target_compile_definitions(avd_assets PRIVATE
    $<$<CONFIG:Debug>:DEBUG>          # Define DEBUG for Debug builds
    $<$<NOT:$<CONFIG:Debug>>:RELEASE> # Define RELEASE for non-Debug builds
)

if (WIN32)
    target_compile_definitions(avd_assets
        PUBLIC _CRT_SECURE_NO_WARNINGS
    )
endif()