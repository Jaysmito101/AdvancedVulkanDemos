# glob all the headers(hpp) inside the ps directory
file(GLOB_RECURSE ps_assets_headers
    ./generated/src/*.h
)

file(GLOB_RECURSE ps_assets_sources
    ./generated/src/*.c
)

# if no source files are found, throw an error
if (NOT ps_assets_sources)
    message(FATAL_ERROR "No source files found in the ps_assets directory! Maybe try running the asset generation script.")
endif()


add_library(ps_assets STATIC
    ${ps_assets_headers}
    ${ps_assets_sources}
)

target_include_directories(ps_assets
    PRIVATE ./generated/include
)

target_compile_definitions(ps_assets PRIVATE
    $<$<CONFIG:Debug>:PS_DEBUG>          # Define PS_DEBUG for Debug builds
    $<$<NOT:$<CONFIG:Debug>>:PS_RELEASE> # Define PS_RELEASE for non-Debug builds
)

if (WIN32)
    target_compile_definitions(ps_assets
        PUBLIC _CRT_SECURE_NO_WARNINGS
    )
endif()