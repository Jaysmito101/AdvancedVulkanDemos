file(GLOB_RECURSE avd_headers
    ./*.h
)

add_executable(avd
    ${avd_headers}
    ./src/avd_main.c
    ./src/avd_application.c
    ./src/avd_window.c
    ./src/avd_vulkan.c
    ./src/avd_vulkan_swapchain.c
    ./src/avd_vulkan_renderer.c
    ./src/avd_vulkan_presentation.c
    ./src/avd_vulkan_framebuffer.c
    ./src/avd_vulkan_image.c
    ./src/avd_vulkan_buffer.c
    ./src/avd_shader_compiler.c
    ./src/avd_utils.c
    ./src/avd_shader.c
    ./src/avd_input.c
    ./src/avd_third_party_impls.c
    ./src/avd_pipeline_utils.c
    ./src/avd_font_renderer.c
    ./src/avd_bloom.c

    ./src/scenes/avd_scenes.c
    ./src/scenes/avd_scenes_splash.c
    ./src/scenes/avd_scenes_loading.c
    ./src/scenes/avd_scenes_mainmenu.c
    ./src/scenes/avd_scenes_prologue.c
)

include_directories(avd
    ./
    ./include
    ../dep/glfw/include
    ../dep/stb
    ../dep/cgltf
    ../dep/cute_headers
    ../avd_assets/generated/include
    ${Vulkan_INCLUDE_DIRS}
)


target_link_libraries(avd
    avd_assets
    glfw
    ${Vulkan_LIBRARY}
    debug $ENV{VULKAN_SDK}/Lib/shaderc_combinedd.lib
    optimized $ENV{VULKAN_SDK}/Lib/shaderc_combined.lib
)


if (WIN32)
    target_compile_definitions(avd PRIVATE _CRT_SECURE_NO_WARNINGS)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ignore:4099")
endif()

add_dependencies(avd generate_assets)

target_compile_definitions(avd PRIVATE
    $<$<CONFIG:Debug>:PS_DEBUG>          # Define PS_DEBUG for Debug builds
    $<$<NOT:$<CONFIG:Debug>>:PS_RELEASE> # Define PS_RELEASE for non-Debug builds
)
