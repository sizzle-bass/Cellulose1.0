# RunPiPLtool.cmake
# -----------------
# Invokes PiPLtool.exe via execute_process so CMake handles quoting correctly.
# Called by add_custom_command -P so paths with spaces are handled properly.
#
# Usage:
#   cmake -DTOOL=<PiPLtool.exe> -DINPUT=<.rr file> -DOUTPUT=<.rrc file> -P RunPiPLtool.cmake

if(NOT TOOL OR NOT INPUT OR NOT OUTPUT)
    message(FATAL_ERROR "RunPiPLtool.cmake: TOOL, INPUT, and OUTPUT must be set via -D")
endif()

execute_process(
    COMMAND "${TOOL}" "${INPUT}" "${OUTPUT}"
    RESULT_VARIABLE result
    ERROR_VARIABLE  err_out
)

if(result)
    message(FATAL_ERROR "PiPLtool failed (exit ${result}):\n${err_out}")
endif()
