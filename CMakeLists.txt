cmake_minimum_required(VERSION 3.20)
project(Cellulose VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(CELLULOSE_ENABLE_CUDA  "Build CUDA GPU path (Windows/NVIDIA)" OFF)
option(CELLULOSE_ENABLE_METAL "Build Metal GPU path (macOS)"         ON)

# ---------------------------------------------------------------------------
# Find Adobe AE SDK
# ---------------------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Default: look for the SDK inside our sdk/ directory.
# The downloaded SDK extracts into sdk/AfterEffectsSDK_*/ae*.AfterEffectsSDK/Examples/
if(NOT AESDK_ROOT)
    # Try to locate the 'Examples' folder automatically inside sdk/
    file(GLOB_RECURSE _aesdk_marker
         "${CMAKE_CURRENT_SOURCE_DIR}/sdk/*/Headers/AE_Effect.h")
    if(_aesdk_marker)
        list(GET _aesdk_marker 0 _first)
        get_filename_component(AESDK_ROOT "${_first}" DIRECTORY)  # …/Headers
        get_filename_component(AESDK_ROOT "${AESDK_ROOT}" DIRECTORY) # …/Examples
        message(STATUS "Auto-detected AESDK_ROOT: ${AESDK_ROOT}")
    endif()
    set(AESDK_ROOT "${AESDK_ROOT}" CACHE PATH "AE SDK root (Examples/ folder)")
endif()

find_package(AESDK REQUIRED)

# ---------------------------------------------------------------------------
# Core source files
# ---------------------------------------------------------------------------
set(CELLULOSE_SOURCES
    src/Cellulose.cpp
    src/CelluloseCPU.cpp
    src/CelluloseGPU.cpp
    "${AESDK_UTIL_SOURCES}"   # Smart_Utils.cpp from SDK Util/
)

# ---------------------------------------------------------------------------
# Platform: Windows — CUDA + PiPL resource
# ---------------------------------------------------------------------------
if(WIN32)
    # --- Optional CUDA ---
    if(CELLULOSE_ENABLE_CUDA)
        include(CheckLanguage)
        check_language(CUDA)
        if(CMAKE_CUDA_COMPILER)
            enable_language(CUDA)
            set(CMAKE_CUDA_STANDARD 17)
            set(CMAKE_CUDA_STANDARD_REQUIRED ON)
            list(APPEND CELLULOSE_SOURCES src/cuda/CelluloseCUDA.cu)
            set(CELLULOSE_HAS_CUDA TRUE)
            message(STATUS "Cellulose: CUDA enabled (${CMAKE_CUDA_COMPILER_VERSION})")
        else()
            message(STATUS "Cellulose: CUDA compiler not found — CPU-only build")
        endif()
    endif()

    # --- PiPL resource ---
    # CellulosePiPL.rc is pre-compiled (PiPLtool cannot handle paths with spaces
    # at build time). Re-run build_pipl.bat if Cellulose.r changes.
    set(PIPL_RC "${CMAKE_CURRENT_SOURCE_DIR}/src/CellulosePiPL.rc")
    if(NOT EXISTS "${PIPL_RC}")
        message(FATAL_ERROR
            "src/CellulosePiPL.rc not found.\n"
            "Run build_pipl.bat from the project root to generate it.")
    endif()
    list(APPEND CELLULOSE_SOURCES "${PIPL_RC}")
endif()

# ---------------------------------------------------------------------------
# Platform: macOS — Metal
# ---------------------------------------------------------------------------
if(APPLE AND CELLULOSE_ENABLE_METAL)
    list(APPEND CELLULOSE_SOURCES src/metal/CelluloseMetal.mm)
    set(CELLULOSE_HAS_METAL TRUE)
    message(STATUS "Cellulose: Metal enabled")
endif()

# ---------------------------------------------------------------------------
# Plugin target
# ---------------------------------------------------------------------------
if(WIN32)
    add_library(Cellulose SHARED ${CELLULOSE_SOURCES})
    set_target_properties(Cellulose PROPERTIES
        OUTPUT_NAME "Cellulose"
        SUFFIX      ".aex"
        PREFIX      ""
    )
elseif(APPLE)
    add_library(Cellulose MODULE ${CELLULOSE_SOURCES})
    set_target_properties(Cellulose PROPERTIES
        OUTPUT_NAME "Cellulose"
        SUFFIX      ".plugin"
        PREFIX      ""
        BUNDLE      TRUE
        OSX_ARCHITECTURES "arm64;x86_64"
    )
else()
    add_library(Cellulose SHARED ${CELLULOSE_SOURCES})
endif()

# ---------------------------------------------------------------------------
# Include directories & link libraries
# ---------------------------------------------------------------------------
target_include_directories(Cellulose PRIVATE src/ src/cuda/ src/metal/)
target_link_libraries(Cellulose PRIVATE AESDK::AESDK)

# ---------------------------------------------------------------------------
# Compile definitions
# ---------------------------------------------------------------------------
if(WIN32)
    target_compile_definitions(Cellulose PRIVATE
        MSWindows WIN32 _WINDOWS _CRT_SECURE_NO_WARNINGS)
endif()

if(CELLULOSE_HAS_CUDA)
    target_compile_definitions(Cellulose PRIVATE CELLULOSE_CUDA_ENABLED)
    find_package(CUDAToolkit REQUIRED)
    target_link_libraries(Cellulose PRIVATE CUDA::cudart)
endif()

if(CELLULOSE_HAS_METAL)
    target_compile_definitions(Cellulose PRIVATE CELLULOSE_METAL_ENABLED)
    target_link_libraries(Cellulose PRIVATE
        "-framework Metal"
        "-framework Foundation"
        "-framework CoreGraphics"
    )

    set(METAL_SRC    "${CMAKE_CURRENT_SOURCE_DIR}/src/metal/CelluloseMetal.metal")
    set(METAL_AIR    "${CMAKE_CURRENT_BINARY_DIR}/CelluloseMetal.air")
    set(METALLIB     "${CMAKE_CURRENT_BINARY_DIR}/CelluloseMetal.metallib")

    add_custom_command(
        OUTPUT  "${METALLIB}"
        COMMAND xcrun -sdk macosx metal -c "${METAL_SRC}" -o "${METAL_AIR}"
        COMMAND xcrun -sdk macosx metallib "${METAL_AIR}" -o "${METALLIB}"
        DEPENDS "${METAL_SRC}"
        COMMENT "Compiling Metal shader"
    )
    add_custom_target(MetalShader DEPENDS "${METALLIB}")
    add_dependencies(Cellulose MetalShader)
    target_compile_definitions(Cellulose PRIVATE
        CELLULOSE_METALLIB_PATH="${METALLIB}")
endif()

# ---------------------------------------------------------------------------
# MSVC flags
# ---------------------------------------------------------------------------
if(MSVC)
    target_compile_options(Cellulose PRIVATE /W3 /MP /EHa)
endif()

# ---------------------------------------------------------------------------
# Installation reminder
# ---------------------------------------------------------------------------
message(STATUS "")
message(STATUS "Cellulose configured. Build with:")
message(STATUS "  cmake --build build --config Release")
if(WIN32)
    message(STATUS "Output: build/Release/Cellulose.aex")
elseif(APPLE)
    message(STATUS "Output: build/Cellulose.plugin")
endif()
message(STATUS "")
