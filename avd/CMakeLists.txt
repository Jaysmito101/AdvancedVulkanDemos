file(GLOB_RECURSE avd_headers
    ./*.h
)

add_executable(avd
    ${avd_headers}
    ./src/core/avd_utils.c
    ./src/core/avd_window.c
    ./src/core/avd_window_cb.c
    ./src/core/avd_input.c

    ./src/avd_main.c
    ./src/avd_application.c

    ./src/deps/avd_third_party_impls.c
    
    ./src/font/avd_font_renderer.c
    
    ./src/bloom/avd_bloom.c

    ./src/shader/avd_shader_compiler.c
    ./src/shader/avd_shader.c

    ./src/vulkan/avd_vulkan_pipeline_utils.c
    ./src/vulkan/avd_vulkan.c
    ./src/vulkan/avd_vulkan_swapchain.c
    ./src/vulkan/avd_vulkan_renderer.c
    ./src/vulkan/avd_vulkan_presentation.c
    ./src/vulkan/avd_vulkan_framebuffer.c
    ./src/vulkan/avd_vulkan_image.c
    ./src/vulkan/avd_vulkan_buffer.c
    
    ./src/ui/avd_ui.c

    ./src/scenes/avd_scenes.c
    ./src/scenes/avd_scenes_main_menu.c
    ./src/scenes/bloom/avd_scenes_bloom.c
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
    $<$<CONFIG:Debug>:AVD_DEBUG>          # Define AVD_DEBUG for Debug builds
    $<$<NOT:$<CONFIG:Debug>>:AVD_RELEASE> # Define AVD_RELEASE for non-Debug builds
)
