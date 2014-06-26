#  How to use this cmake file. ( Note: later we will integrate it with the main CMakeLists)
# > mdkir build
# > cd build
# > cmake -DCMAKE_VERBOSE_MAKEFILE=1 ..
# > make

cmake_minimum_required(VERSION 2.8)
#
#  1.  MongoDB connector 
#
FILE(GLOB_RECURSE MONGO_DB_CONNECTOR_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "mongo/*.cpp")

MESSAGE(STATUS "This is SOURCE dir " ${CMAKE_SOURCE_DIR})
					
INCLUDE_DIRECTORIES(
    #/usr/include/mongo/
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/../src/adapter/
)

ADD_LIBRARY(mongodbConnector SHARED ${MONGO_DB_CONNECTOR_SRC} )

SET_TARGET_PROPERTIES(mongodbConnector PROPERTIES COMPILE_FLAGS -fPIC)

set(MONGO_CLIENT_LIBRARY ${CMAKE_SOURCE_DIR}/lib/libmongoclient.so)
TARGET_LINK_LIBRARIES(mongodbConnector ${MONGO_CLIENT_LIBRARY} -lboost_system -pthread -lboost_filesystem -lboost_program_options)