# PreprocessR.cmake
# -----------------
# Helper script invoked by CMake add_custom_command to preprocess a PiPL .r file.
# Uses execute_process so no shell redirection is needed.
#
# Usage (called by CMakeLists.txt add_custom_command -P):
#   cmake -DCXX_COMPILER=<cl.exe path>
#         -DSDK_INC=<Headers dir>
#         -DINPUT=<input file>
#         -DOUTPUT=<output file>
#         [-DDEFINES=MSWindows]
#         -P PreprocessR.cmake

if(NOT CXX_COMPILER OR NOT SDK_INC OR NOT INPUT OR NOT OUTPUT)
    message(FATAL_ERROR "PreprocessR.cmake: missing required variable (CXX_COMPILER, SDK_INC, INPUT, OUTPUT)")
endif()

# Build compile arguments
set(ARGS /nologo /I "${SDK_INC}" /EP "${INPUT}")
if(DEFINES)
    list(APPEND ARGS /D "${DEFINES}")
endif()

execute_process(
    COMMAND "${CXX_COMPILER}" ${ARGS}
    OUTPUT_FILE  "${OUTPUT}"
    ERROR_VARIABLE preprocess_err
    RESULT_VARIABLE preprocess_result
)

if(preprocess_result)
    message(FATAL_ERROR "PiPL preprocessing failed:\n${preprocess_err}")
endif()
