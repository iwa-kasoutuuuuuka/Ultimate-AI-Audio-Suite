cmake_minimum_required(VERSION 3.10)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(ResembleDebugV60)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ONNX Runtime (DirectML Version)
set(ONNX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../packages/Microsoft.ML.OnnxRuntime.DirectML.1.20.1")
set(DML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../packages/Microsoft.AI.DirectML.1.15.2")

# Fetch dependencies
include(FetchContent)
FetchContent_Declare(glfw URL https://github.com/glfw/glfw/releases/download/3.3.8/glfw-3.3.8.zip)
FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG v1.90.1)
FetchContent_Declare(implot GIT_REPOSITORY https://github.com/epezent/implot.git GIT_TAG v0.16)

set(FETCHCONTENT_QUIET OFF)
FetchContent_MakeAvailable(glfw imgui implot)

# Paths
set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMPLOT_DIR ${implot_SOURCE_DIR})

include_directories(
    include
    ${glfw_SOURCE_DIR}/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${IMPLOT_DIR}
    "f:/antigravity/miniconda/Lib/site-packages/torch/include"
    "f:/antigravity/miniconda/Lib/site-packages/torch/include/torch/csrc/api/include"
    "${ONNX_DIR}/build/native/include"
    "f:/antigravity/ffmpeg_dev/ffmpeg-master-latest-win64-gpl-shared/include"
)

link_directories(
    "f:/antigravity/miniconda/Lib/site-packages/torch/lib"
    "f:/antigravity/ffmpeg_dev/ffmpeg-master-latest-win64-gpl-shared/lib"
    "${ONNX_DIR}/runtimes/win-x64/native"
)

file(GLOB SOURCES 
    "src/*.cpp"
    "${IMGUI_DIR}/imgui*.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
    "${IMPLOT_DIR}/implot*.cpp"
    "resources.rc"
)

add_executable(ResembleDebugV60 ${SOURCES})

target_link_libraries(ResembleDebugV60 PRIVATE 
    glfw
    opengl32
    winmm
    ole32
    uuid
    torch torch_cpu c10
    "${ONNX_DIR}/runtimes/win-x64/native/onnxruntime.lib"
    avformat avcodec avutil swresample
)

if(MSVC)
    add_compile_options("/utf-8")
endif()

set_property(TARGET ResembleDebugV60 PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# Copy DLLs to output
add_custom_command(TARGET ResembleDebugV60 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ONNX_DIR}/runtimes/win-x64/native/onnxruntime.dll"
        $<TARGET_FILE_DIR:ResembleDebugV60>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${DML_DIR}/bin/x64-win/DirectML.dll"
        $<TARGET_FILE_DIR:ResembleDebugV60>
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:ResembleDebugV60>/bin
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "f:/antigravity/cpp_src/bin/ffmpeg.exe"
        $<TARGET_FILE_DIR:ResembleDebugV60>/bin/
)
