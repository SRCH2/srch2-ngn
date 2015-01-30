#  Oracle connector 
#  How to use this cmake file.
# > mdkir build
# > cd build
# > cmake ..
# > make

cmake_minimum_required(VERSION 2.8)

FILE(GLOB_RECURSE ORACLE_CONNECTOR_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "oracle/*.cpp" "util/*.cpp")

INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/
    ${CMAKE_SOURCE_DIR}/oracle/
    ${CMAKE_SOURCE_DIR}/thirdparty/json/jsoncpp-src/include
    ${CMAKE_SOURCE_DIR}/util/
)

SET( jsoncpp_LIBRARY ${CMAKE_SOURCE_DIR}/thirdparty/json/jsoncpp-src/build/libjsoncpp.a)

ADD_LIBRARY(oracleConnector SHARED ${ORACLE_CONNECTOR_SRC} )

set(ORACLE_VERSION_MAJOR 1)
set(ORACLE_VERSION_MINOR 0)
set(ORACLE_VERSION_PATCH 0)
set(ORACLE_VERSION_STRING ${ORACLE_VERSION_MAJOR}.${ORACLE_VERSION_MINOR}.${ORACLE_VERSION_PATCH})

set_target_properties(oracleConnector PROPERTIES VERSION ${ORACLE_VERSION_STRING}
                                  SOVERSION ${ORACLE_VERSION_MAJOR})
                                  
SET_TARGET_PROPERTIES(oracleConnector PROPERTIES COMPILE_FLAGS -fPIC)

TARGET_LINK_LIBRARIES(oracleConnector ${jsoncpp_LIBRARY} -lodbc)
