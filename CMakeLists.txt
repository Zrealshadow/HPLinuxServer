cmake_minimum_required(VERSION 3.0.0)
project(HPLinuxWebServer VERSION 0.1.0)

include(CTest)
enable_testing()

SET(SRC_DIR_PATH ${PROJECT_SOURCE_DIR}/src)
SET(EXECUTION_SRC_DIR ${SRC_DIR_PATH}/execution)


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)


add_executable(byteorder ${EXECUTION_SRC_DIR}/byteorder.c)
add_executable(checkBacklog ${EXECUTION_SRC_DIR}/checkBacklog.c)
add_executable(acceptSocket ${EXECUTION_SRC_DIR}/acceptSocket.c)