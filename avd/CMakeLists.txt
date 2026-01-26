file(GLOB_RECURSE avd_headers
    ./*.h
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


add_executable(avd
    ${avd_headers}
    ./src/core/avd_utils.c
    ./src/core/avd_utils_curl.c
    ./src/core/avd_utils_curl_tests.c
    ./src/core/avd_window.c
    ./src/core/avd_window_cb.c
    ./src/core/avd_list.c
    ./src/core/avd_list_tests.c
    ./src/core/avd_hashtable.c
    ./src/core/avd_hashtable_tests.c
    ./src/core/avd_input.c
    ./src/core/avd_aligned_buffer.c

    ./src/avd_main.c
    ./src/avd_application.c

    ./src/deps/avd_third_party_impls.c

    ./src/font/avd_font_renderer.c

    ./src/common/avd_bloom.c
    ./src/common/avd_eyeball.c

    ./src/shader/avd_shader_shaderc.c
    ./src/shader/avd_shader_slang.c
    ./src/shader/avd_shader_base.c
    ./src/shader/avd_shader.c

    ./src/vulkan/avd_vulkan_pipeline_utils.c
    ./src/vulkan/avd_vulkan.c
    ./src/vulkan/avd_vulkan_swapchain.c
    ./src/vulkan/avd_vulkan_renderer.c
    ./src/vulkan/avd_vulkan_presentation.c
    ./src/vulkan/avd_vulkan_framebuffer.c
    ./src/vulkan/avd_vulkan_image.c
    ./src/vulkan/avd_vulkan_buffer.c
    ./src/vulkan/avd_vulkan_debug.c
    ./src/vulkan/avd_vulkan_video_decoder.c
    ./src/vulkan/avd_vulkan_h264_video.c

    ./src/ui/avd_ui.c

    ./src/math/avd_matrix_non_simd.c
    ./src/math/avd_quaternion_non_simd.c
    ./src/math/avd_math_tests.c

    ./src/geom/avd_3d_matrices.c
    ./src/geom/avd_transform.c

    ./src/model/avd_model_base.c
    ./src/model/avd_model.c
    ./src/model/avd_3d_scene.c
    ./src/model/avd_model_obj_loader.c
    ./src/model/avd_model_gltf_loader.c
    ./src/model/avd_meshgen.c

    ./src/scenes/avd_scenes.c
    ./src/scenes/avd_scenes_main_menu.c
    ./src/scenes/bloom/avd_scenes_bloom.c
    ./src/scenes/2d_radiance_cascades/avd_scenes_2d_radiance_cascades.c
    ./src/scenes/deccer_cubes/avd_scenes_deccer_cubes.c
    ./src/scenes/subsurface_scattering/avd_scenes_subsurface_scattering.c
    ./src/scenes/eyeballs/avd_scenes_eyeballs.c
    ./src/scenes/realistic_head/avd_scenes_realistic_head.c
    ./src/scenes/hls_player/avd_scenes_hls_player.c
)

include_directories(avd
    ./
    ./include
    ../dep/glfw/include
    ../dep/libpico/include
    ../dep/stb
    ../dep/cgltf
    ../dep/cute_headers
    ../dep/volk
    ../dep/tinyobjloader-c
    ../avd_assets/generated/include
    ${Vulkan_INCLUDE_DIRS}
)

# Ensure the Vulkan SDK path is set properly
if(DEFINED ENV{VULKAN_SDK})
    set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
else()
    message(FATAL_ERROR "VULKAN_SDK environment variable is not set")
endif()


string(REPLACE "\\" "/" VULKAN_SDK_PATH ${VULKAN_SDK_PATH})
message(STATUS "Vulkan SDK Path: ${VULKAN_SDK_PATH}")


target_link_libraries(avd
    avd_assets
    glfw
    volk
    debug ${VULKAN_SDK_PATH}/Lib/shaderc_combinedd.lib
    optimized ${VULKAN_SDK_PATH}/Lib/shaderc_combined.lib
)


if (MSVC)
    target_compile_definitions(avd PRIVATE _CRT_SECURE_NO_WARNINGS)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ignore:4099")
endif()

target_compile_definitions(avd PRIVATE
    $<$<CONFIG:Debug>:AVD_DEBUG>          # Define AVD_DEBUG for Debug builds
    $<$<NOT:$<CONFIG:Debug>>:AVD_RELEASE> # Define AVD_RELEASE for non-Debug builds
)


if (MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_link_options(avd PRIVATE /IGNORE:4099)
    target_compile_options(avd PRIVATE /MP /FS)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND WIN32)
    # Clang must pass MSVC-style flags via -Xlinker
    target_link_options(avd PRIVATE -Xlinker /IGNORE:4099)
endif()