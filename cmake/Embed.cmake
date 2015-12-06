# -*- Mode: CMake; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */

INCLUDE(CMakeParseArguments REQUIRED)

FIND_PROGRAM(RESCUE_BINARY rescue)

MACRO(EMBED_GENERATE)

    SET(SINGLE_OPTIONS TARGET PREFIX DIRECTORY)

    cmake_parse_arguments(EMBED "" "${SINGLE_OPTIONS}" "FILES" ${ARGN})
 
    IF ("${EMBED_PREFIX}" STREQUAL "")
        SET(EMBED_PREFIX "embedded")
    ENDIF()

    IF ("${EMBED_DIRECTORY}" STREQUAL "")
        SET(EMBED_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    ENDIF()

    add_custom_command(OUTPUT ${EMBED_TARGET}
                   COMMAND ${RESCUE_BINARY} ARGS -o ${EMBED_TARGET} -p ${EMBED_PREFIX} ${EMBED_FILES}
                   DEPENDS ${EMBED_FILES}
                   WORKING_DIRECTORY ${EMBED_DIRECTORY}
                   COMMENT "Generating resources file")

ENDMACRO(EMBED_GENERATE)
