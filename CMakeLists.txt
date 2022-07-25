cmake_minimum_required(VERSION 3.0.0)
project(HPLinuxWebServer VERSION 0.1.0)

include(CTest)
enable_testing()

SET(SRC_DIR_PATH ${PROJECT_SOURCE_DIR}/src)
SET(EXECUTION_SRC_DIR ${SRC_DIR_PATH}/execution)
SET(INCLUDE_SRC_DIR ${SRC_DIR_PATH}/include)
SET(LIB_DIR_PATH ${PROJECT_SOURCE_DIR}/lib)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# add_executable(byteorder ${EXECUTION_SRC_DIR}/byteorder.c)
# add_executable(checkBacklog ${EXECUTION_SRC_DIR}/checkBacklog.c)
# add_executable(acceptSocket ${EXECUTION_SRC_DIR}/acceptSocket.c)
file(GLOB_RECURSE execute_files RELATIVE "${EXECUTION_SRC_DIR}" "${EXECUTION_SRC_DIR}/*.c")

foreach(target_file IN LISTS execute_files)
    message("generate target from file ${target_file}")
    get_filename_component(target_name ${target_file} NAME_WE)
    add_executable(${target_name} ${EXECUTION_SRC_DIR}/${target_file})
endforeach()

target_link_libraries(epoll_oneshot
    pthread
)

message("${EXECUTION_SRC_DIR}/timeout_server.cpp")

# add timout_server
add_executable(timeout_server "${EXECUTION_SRC_DIR}/timeout_server.cpp")

target_include_directories(timeout_server PUBLIC ${INCLUDE_SRC_DIR})
target_link_libraries(timeout_server
    pthread
)

add_executable(subprocess_chat_room "${EXECUTION_SRC_DIR}/subprocess_chat_room.cpp")
target_include_directories(subprocess_chat_room PUBLIC ${INCLUDE_SRC_DIR})
target_link_libraries(subprocess_chat_room
    pthread
    rt
)

add_executable(echo_processpool_server "${EXECUTION_SRC_DIR}/echo_processpool_server.cpp")
target_include_directories(echo_processpool_server PUBLIC ${INCLUDE_SRC_DIR})

# message(${INCLUDE_SRC_DIR})
# message("${SRC_DIR_PATH}/httpConn")
file(GLOB HTTPCONN_SOURCES
    "${SRC_DIR_PATH}/httpConn/*"
    "${INCLUDE_SRC_DIR}/*.h"
)

# message(${HTTPCONN_SOURCES})

add_executable(http_server "${HTTPCONN_SOURCES}")
target_include_directories(http_server
    PUBLIC
    ${INCLUDE_SRC_DIR} "${SRC_DIR_PATH}/httpConn")

target_link_libraries(http_server
    pthread
)
