# FindAESDK.cmake
# ----------------
# Locates the Adobe After Effects SDK.
#
# Search order:
#   1. CMake variable AESDK_ROOT  (set via -DAESDK_ROOT=<path>)
#   2. Environment variable AESDK_ROOT
#
# Imported target: AESDK::AESDK
#
# Exported variables:
#   AESDK_FOUND            — TRUE when the SDK is found
#   AESDK_INCLUDE_DIRS     — Headers/ and Util/ directories
#   AESDK_RESOURCES_DIR    — Resources/ directory (PiPL tool + templates)
#   AESDK_UTIL_SOURCES     — Smart_Utils.cpp (must be compiled into the plugin)

cmake_minimum_required(VERSION 3.20)

if(NOT AESDK_ROOT)
    if(DEFINED ENV{AESDK_ROOT})
        set(AESDK_ROOT "$ENV{AESDK_ROOT}" CACHE PATH "Adobe AE SDK root directory")
    endif()
endif()

# Find Headers/
find_path(AESDK_INCLUDE_DIR
    NAMES AE_Effect.h
    PATHS
        "${AESDK_ROOT}/Headers"
        "${AESDK_ROOT}/headers"
    NO_DEFAULT_PATH
    DOC "Adobe AE SDK Headers directory"
)

# Find Util/ (AEFX_SuiteHelper.h, Smart_Utils.h, AEGP_SuiteHandler.h)
find_path(AESDK_UTIL_DIR
    NAMES AEFX_SuiteHelper.h
    PATHS
        "${AESDK_ROOT}/Util"
        "${AESDK_ROOT}/util"
    NO_DEFAULT_PATH
    DOC "Adobe AE SDK Util directory"
)

# Find Resources/ (PiPLtool.exe + AE_General.r)
find_path(AESDK_RESOURCES_DIR
    NAMES PiPLtool.exe PiPLtool
    PATHS
        "${AESDK_ROOT}/Resources"
        "${AESDK_ROOT}/resources"
    NO_DEFAULT_PATH
    DOC "Adobe AE SDK Resources directory"
)

# Smart_Utils.cpp must be compiled into the plugin for UnionLRect etc.
find_file(AESDK_SMART_UTILS_CPP
    NAMES Smart_Utils.cpp
    PATHS "${AESDK_ROOT}/Util"
    NO_DEFAULT_PATH
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AESDK
    REQUIRED_VARS AESDK_INCLUDE_DIR AESDK_UTIL_DIR
    FAIL_MESSAGE "Adobe AE SDK not found. Set AESDK_ROOT to the Examples/ folder inside the SDK."
)

if(AESDK_FOUND AND NOT TARGET AESDK::AESDK)
    add_library(AESDK::AESDK INTERFACE IMPORTED)
    set_target_properties(AESDK::AESDK PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${AESDK_INCLUDE_DIR};${AESDK_INCLUDE_DIR}/SP;${AESDK_UTIL_DIR}"
    )
    set(AESDK_UTIL_SOURCES "${AESDK_SMART_UTILS_CPP}" CACHE FILEPATH "Smart_Utils.cpp path")
    set(AESDK_RESOURCES_DIR "${AESDK_RESOURCES_DIR}" CACHE PATH "AE SDK Resources dir" FORCE)
endif()

mark_as_advanced(AESDK_INCLUDE_DIR AESDK_UTIL_DIR AESDK_RESOURCES_DIR AESDK_SMART_UTILS_CPP)
