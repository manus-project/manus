
PROJECT(manus)
CMAKE_MINIMUM_REQUIRED(VERSION 3.1 FATAL_ERROR)

include(GNUInstallDirs)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
SET(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
SET(CMAKE_CXX_STANDARD 11)

FIND_PROGRAM(ECHOGEN echogen)

ADD_CUSTOM_TARGET(manus_messages ALL
                DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/messages/manus/manipulator.h
                COMMENT "Generating message wrappers")

ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/messages/manus/manipulator.h
                COMMAND ${ECHOGEN}
                --python-outdir ${CMAKE_CURRENT_BINARY_DIR}/python/manus/
                --cpp-outdir ${CMAKE_CURRENT_BINARY_DIR}/messages/manus/
                ${CMAKE_SOURCE_DIR}/msg/manipulator.msg
                ${CMAKE_SOURCE_DIR}/msg/apps.msg
                DEPENDS ${CMAKE_SOURCE_DIR}/msg/manipulator.msg
                COMMENT "Processing message descriptions")

FIND_PACKAGE(echolib REQUIRED)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/src/ ${CMAKE_CURRENT_BINARY_DIR}/messages/ ${echolib_INCLUDE_DIRS} include/)

ADD_EXECUTABLE(manus_simulator_manipulator src/manipulator.cpp src/simulation.cpp)
TARGET_LINK_LIBRARIES(manus_simulator_manipulator ${echolib_LIBRARIES})
add_dependencies(manus_simulator_manipulator manus_messages)

ADD_EXECUTABLE(manus_serial_manipulator src/manipulator.cpp src/serial.cpp src/debug.cpp)
TARGET_LINK_LIBRARIES(manus_serial_manipulator ${echolib_LIBRARIES})
add_dependencies(manus_serial_manipulator manus_messages)

find_path(EIGEN3_INCLUDE_DIR NAMES signature_of_eigen3_matrix_library PATHS ${CMAKE_INSTALL_PREFIX}/include PATH_SUFFIXES eigen3 eigen)
FIND_PACKAGE(orocos_kdl REQUIRED)
ADD_EXECUTABLE(manus_planner src/planner.cpp src/voxelgrid.cpp)
TARGET_INCLUDE_DIRECTORIES(manus_planner PUBLIC ${orocos_kdl_INCLUDE_DIRS} ${EIGEN3_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(manus_planner ${echolib_LIBRARIES} ${orocos_kdl_LIBRARIES})

ADD_SUBDIRECTORY(python)

install(TARGETS manus_simulator_manipulator manus_serial_manipulator manus_planner DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/messages/manus/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/manus
)


