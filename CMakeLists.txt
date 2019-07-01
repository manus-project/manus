
PROJECT(manus)
CMAKE_MINIMUM_REQUIRED(VERSION 3.1 FATAL_ERROR)

include(GNUInstallDirs)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
SET(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
SET(CMAKE_CXX_STANDARD 11)

OPTION(BUILD_MPSSE "Build with support for MPSSE devices" OFF)
option(BUILD_DEBUG "Enable debug output" OFF)

if(BUILD_DEBUG)
    add_definitions(-DMANUS_DEBUG)
    add_definitions(-DMANUS_SOURCE_COMPILE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/src/")
endif()

FIND_PROGRAM(ECHOGEN echogen)

ADD_CUSTOM_TARGET(manus_messages ALL
                DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/messages/manus/messages.h
                COMMENT "Generating message wrappers")

ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/messages/manus/messages.h
                COMMAND ${ECHOGEN}
                --python-outdir ${CMAKE_CURRENT_BINARY_DIR}/python/manus/
                --cpp-outdir ${CMAKE_CURRENT_BINARY_DIR}/messages/manus/
                -p ${CMAKE_SOURCE_DIR}/msg/
                ${CMAKE_SOURCE_DIR}/msg/messages.msg
                DEPENDS 
                ${CMAKE_SOURCE_DIR}/msg/messages.msg
                ${CMAKE_SOURCE_DIR}/msg/manipulator.msg 
                ${CMAKE_SOURCE_DIR}/msg/apps.msg
                ${CMAKE_SOURCE_DIR}/msg/privileged.msg
                ${CMAKE_SOURCE_DIR}/msg/markers.msg
                COMMENT "Processing message descriptions")

FIND_PACKAGE(echolib REQUIRED)
FIND_PACKAGE(echocv REQUIRED)
FIND_PACKAGE(OpenServo REQUIRED)
FIND_PACKAGE(yaml-cpp REQUIRED)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/src/ ${CMAKE_CURRENT_BINARY_DIR}/messages/ ${echolib_INCLUDE_DIRS} include/)

ADD_EXECUTABLE(manus_simulator_manipulator src/manipulator.cpp src/simulation.cpp src/files.cpp)
TARGET_LINK_LIBRARIES(manus_simulator_manipulator ${echolib_LIBRARIES} ${YAML_CPP_LIBRARIES})
add_dependencies(manus_simulator_manipulator manus_messages)

IF (BUILD_MPSSE)
    FIND_PACKAGE(LibFTDI1 REQUIRED)
    INCLUDE_DIRECTORIES(${LIBFTDI_INCLUDE_DIRS})
ENDIF(BUILD_MPSSE)

ADD_EXECUTABLE(manus_openservo_manipulator src/manipulator.cpp src/openservorobot.cpp src/debug.cpp src/files.cpp)
TARGET_LINK_LIBRARIES(manus_openservo_manipulator ${echolib_LIBRARIES} ${OpenServo_LIBRARIES} ${YAML_CPP_LIBRARIES} ${LIBFTDI_LIBRARIES})
add_dependencies(manus_openservo_manipulator manus_messages)

ADD_EXECUTABLE(manus_openservo_calibration src/calibration.cpp)
TARGET_LINK_LIBRARIES(manus_openservo_calibration ${OpenServo_LIBRARIES} ${YAML_CPP_LIBRARIES} ${LIBFTDI_LIBRARIES})

if(BUILD_DEBUG)
FIND_PACKAGE(OpenCV REQUIRED core imgproc video calib3d)
else()
FIND_PACKAGE(OpenCV REQUIRED core imgproc video calib3d highgui)
endif()

FIND_PACKAGE(Ary REQUIRED)
ADD_EXECUTABLE(manus_artrack src/artrack.cpp)
INCLUDE_DIRECTORIES(${OpenCV_INCLUDE_DIRS} ${ary_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(manus_artrack ${echolib_LIBRARIES} ${echocv_LIBRARIES} ${OpenCV_LIBS} ${Ary_LIBRARIES})

find_path(EIGEN3_INCLUDE_DIR NAMES signature_of_eigen3_matrix_library PATHS ${CMAKE_INSTALL_PREFIX}/include PATH_SUFFIXES eigen3 eigen)
FIND_PACKAGE(orocos_kdl REQUIRED)
ADD_EXECUTABLE(manus_planner src/planner.cpp src/voxelgrid.cpp src/files.cpp)
TARGET_INCLUDE_DIRECTORIES(manus_planner PUBLIC ${orocos_kdl_INCLUDE_DIRS} ${EIGEN3_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(manus_planner ${echolib_LIBRARIES} ${orocos_kdl_LIBRARIES} ${YAML_CPP_LIBRARIES})
add_dependencies(manus_planner manus_messages)

ADD_SUBDIRECTORY(python)

install(TARGETS 
        manus_simulator_manipulator
        manus_openservo_manipulator
        manus_openservo_calibration 
        manus_planner
        manus_artrack
        DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/messages/manus/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/manus
)


