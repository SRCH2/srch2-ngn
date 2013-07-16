# Install script for directory: /home/jianfeng/workspace/srch2-ngn-jianfeng/include/instantsearch

# Set the install prefix
IF(NOT DEFINED CMAKE_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "/home/jianfeng/software/android-toolchain/user")
ENDIF(NOT DEFINED CMAKE_INSTALL_PREFIX)
STRING(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
IF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  IF(BUILD_TYPE)
    STRING(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  ELSE(BUILD_TYPE)
    SET(CMAKE_INSTALL_CONFIG_NAME "Debug")
  ENDIF(BUILD_TYPE)
  MESSAGE(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
ENDIF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)

# Set the component getting installed.
IF(NOT CMAKE_INSTALL_COMPONENT)
  IF(COMPONENT)
    MESSAGE(STATUS "Install component: \"${COMPONENT}\"")
    SET(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  ELSE(COMPONENT)
    SET(CMAKE_INSTALL_COMPONENT)
  ENDIF(COMPONENT)
ENDIF(NOT CMAKE_INSTALL_COMPONENT)

# Install shared libraries without execute permission?
IF(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  SET(CMAKE_INSTALL_SO_NO_EXE "1")
ENDIF(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/srch2/instantsearch/include/instantsearch" TYPE FILE FILES
    "/home/jianfeng/workspace/srch2-ngn-jianfeng/debug/include/instantsearch/Schema.h"
    "/home/jianfeng/workspace/srch2-ngn-jianfeng/debug/include/instantsearch/GlobalCache.h"
    "/home/jianfeng/workspace/srch2-ngn-jianfeng/debug/include/instantsearch/Record.h"
    "/home/jianfeng/workspace/srch2-ngn-jianfeng/debug/include/instantsearch/IndexSearcher.h"
    "/home/jianfeng/workspace/srch2-ngn-jianfeng/debug/include/instantsearch/Term.h"
    "/home/jianfeng/workspace/srch2-ngn-jianfeng/debug/include/instantsearch/Stat.h"
    "/home/jianfeng/workspace/srch2-ngn-jianfeng/debug/include/instantsearch/Analyzer.h"
    "/home/jianfeng/workspace/srch2-ngn-jianfeng/debug/include/instantsearch/platform.h"
    "/home/jianfeng/workspace/srch2-ngn-jianfeng/debug/include/instantsearch/Ranker.h"
    "/home/jianfeng/workspace/srch2-ngn-jianfeng/debug/include/instantsearch/QueryResults.h"
    "/home/jianfeng/workspace/srch2-ngn-jianfeng/debug/include/instantsearch/Query.h"
    "/home/jianfeng/workspace/srch2-ngn-jianfeng/debug/include/instantsearch/Indexer.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

