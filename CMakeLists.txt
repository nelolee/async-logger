CMAKE_MINIMUM_REQUIRED(VERSION 2.8.0)
PROJECT(async-logger)
STRING(TOUPPER ${CMAKE_BUILD_TYPE} LOGGER_BUILD_TYPE)
MESSAGE(STATUS "LOGGER_BUILD_TYPE ${LOGGER_BUILD_TYPE}")
SET(LOGGER_BIN_OUTPUT "${PROJECT_BINARY_DIR}/")
SET(LOGGER_LIB_OUTPUT "${PROJECT_BINARY_DIR}/")
SET(LOGGER_INCLUDE_PATH "${CMAKE_SOURCE_DIR}/src")
MESSAGE(STATUS "LOGGER_INCLUDE_PATH ${LOGGER_INCLUDE_PATH}")


SET(LOGGER_BIN_OUTPUT "${LOGGER_BIN_OUTPUT}bin")
SET(LOGGER_LIB_OUTPUT "${LOGGER_LIB_OUTPUT}lib")

IF(UNIX)
IF(LOGGER_BUILD_TYPE STREQUAL DEBUG)
SET(LOGGER_BIN_OUTPUT "${LOGGER_BIN_OUTPUT}/debug")
SET(LOGGER_LIB_OUTPUT "${LOGGER_LIB_OUTPUT}/debug")
ELSE()
SET(LOGGER_BIN_OUTPUT "${LOGGER_BIN_OUTPUT}/release")
SET(LOGGER_LIB_OUTPUT "${LOGGER_LIB_OUTPUT}/release")
ENDIF()
ENDIF()
MESSAGE(STATUS "LOGGER_BIN_OUTPUT ${LOGGER_BIN_OUTPUT}")
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -z defs -Wl,-rpath,'$ORIGIN'")#
ENDIF()
ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(test_case)