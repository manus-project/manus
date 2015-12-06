
PROJECT(manus)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8 FATAL_ERROR)
GET_FILENAME_COMPONENT(PROJECT_ROOT . ABSOLUTE)
GET_FILENAME_COMPONENT(BUILD_ROOT ${CMAKE_CURRENT_BINARY_DIR} ABSOLUTE)
SET(CMAKE_MODULE_PATH ${PROJECT_ROOT}/cmake)
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
SET(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
INCLUDE(Embed REQUIRED)

IF(WIN32 OR WIN64)
	ADD_DEFINITIONS(-DPLATFORM_WINDOWS)
#	FIND_PACKAGE(OpenCVWindows REQUIRED)
#	SET(OpenCV_INCLUDE_DIR ${OPENCV_INCLUDE_DIR})
#	SET(OpenCV_LIBS "${OPENCV_CORE_LIBRARY};${OPENCV_IMGPROC_LIBRARY};${OPENCV_VIDEO_LIBRARY};${OPENCV_HIGHGUI_LIBRARY}")

    IF(MINGW)
        SET(MINGW_LIBRARIES -lws2_32 -lrpcrt4 -liphlpapi)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc -static-libstdc++")
        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")

    ENDIF()
ELSEIF(APPLE)  
	ADD_DEFINITIONS(-DPLATFORM_OSX)
# http://www.cmake.org/pipermail/cmake/attachments/20130920/87614932/attachment-0001.txt
	FIND_PACKAGE(OpenCV REQUIRED core imgproc video highgui calib3d)
ELSEIF(UNIX)
    ADD_DEFINITIONS(-DPLATFORM_LINUX)

    IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    ENDIF ()
	FIND_PACKAGE(OpenCV REQUIRED core imgproc video highgui calib3d)
ELSE()
	MESSAGE(FATAL_ERROR "Unrecognized platform")
ENDIF()

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

LINK_LIBRARIES(${MINGW_LIBRARIES})
LINK_LIBRARIES(${OpenCV_LIBS})
INCLUDE_DIRECTORIES(AFTER ${CMAKE_CURRENT_BINARY_DIR} ${OpenCV_INCLUDE_DIR})

ADD_DEFINITIONS(-DMONGOOSE_NO_AUTH -DMONGOOSE_NO_CGI -DMONGOOSE_NO_DAV -DMONGOOSE_NO_USER -DMONGOOSE_NO_WEBSOCKET)

SET(SERVER_SOURCES 
    src/mongoose.c 
    src/threads.c
    src/json.cpp
    src/debug.cpp
    src/server.cpp
    src/manus.cpp
    src/filesystem.cpp
    src/arm.cpp
    src/simulation.cpp
    src/serial.cpp
    src/camera.cpp
    src/pattern.cpp
    src/getopt.c
)

# These resources are always embedded
SET(EMBED_SOURCES 
    res/calibration.xml
    res/marker1.png
    res/marker2.png
    res/surface.pdf
)

# If the program is compiled in Debug mode, then use external web resources for easier development
IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
    ADD_DEFINITIONS(-DSERVER_DOCUMENT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/res")
ELSE(CMAKE_BUILD_TYPE MATCHES DEBUG)
    LIST(APPEND EMBED_SOURCES 
        res/index.html
        res/jquery.js
        res/bootstrap.min.js
        res/manus.js
        res/manus.css
        res/world.js
        res/widgets.js
        res/debug.js
        res/bootstrap.min.css
        res/phoria.min.js
        res/glmatrix.min.js
    )
ENDIF(CMAKE_BUILD_TYPE MATCHES DEBUG)

EMBED_GENERATE(TARGET "${CMAKE_CURRENT_BINARY_DIR}/embedded.c" FILES ${EMBED_SOURCES})
SET(SERVER_SOURCES ${SERVER_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/embedded.c)

ADD_EXECUTABLE(manus ${SERVER_SOURCES})

