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
    /usr/include/mongo/
    ${CMAKE_SOURCE_DIR}/../src/adapter/
)

ADD_LIBRARY(mongodbConnector SHARED ${MONGO_DB_CONNECTOR_SRC} )

SET_TARGET_PROPERTIES(mongodbConnector PROPERTIES COMPILE_FLAGS -fPIC)
TARGET_LINK_LIBRARIES(mongodbConnector -lmongoclient -lboost_system -pthread -lboost_filesystem -lboost_program_options)