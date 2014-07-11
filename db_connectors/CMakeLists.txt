#  How to use this cmake file.
# > mdkir build
# > cd build
# > cmake ..
# > make

cmake_minimum_required(VERSION 2.8)
#
#  1.  MongoDB connector 
#
FILE(GLOB_RECURSE MONGO_DB_CONNECTOR_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "mongo/*.cpp")

MESSAGE(STATUS "This is SOURCE dir " ${CMAKE_SOURCE_DIR})
					
INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/../thirdparty/mongo-cxx-driver/mongo-cxx-driver/build/use-system-boost/srch2/include
    ${CMAKE_SOURCE_DIR}/../src/adapter/
)

ADD_LIBRARY(mongodbConnector SHARED ${MONGO_DB_CONNECTOR_SRC} )

SET_TARGET_PROPERTIES(mongodbConnector PROPERTIES COMPILE_FLAGS -fPIC)

set(MONGO_CLIENT_LIBRARY  ${CMAKE_SOURCE_DIR}/../thirdparty/mongo-cxx-driver/mongo-cxx-driver/build/use-system-boost/libmongoclient.so)

TARGET_LINK_LIBRARIES(mongodbConnector ${MONGO_CLIENT_LIBRARY} -lboost_system -pthread -lboost_filesystem -lboost_program_options)



#
#  2.  SQLite connector 
#

EXEC_PROGRAM( ${CMAKE_CXX_COMPILER}
                      ARGS  -dumpversion
                  OUTPUT_VARIABLE gcc_compiler_version)


FILE(GLOB_RECURSE SQLITE_CONNECTOR_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "sqlite/*.cpp")

MESSAGE(STATUS "This is SOURCE dir " ${CMAKE_SOURCE_DIR})
					
INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/../src/adapter/
    ${CMAKE_SOURCE_DIR}/../thirdparty/json/jsoncpp-src-0.5.0/include
)

#ADD_EXECUTABLE(sqliteTest ${SQLITE_CONNECTOR_SRC})

SET( jsoncpp_LIBRARY ${CMAKE_SOURCE_DIR}/../thirdparty/json/jsoncpp-src-0.5.0/libs/linux-gcc-${gcc_compiler_version}/libjson_linux-gcc-${gcc_compiler_version}_libmt.so)
ADD_LIBRARY(sqliteConnector SHARED ${SQLITE_CONNECTOR_SRC} )

SET_TARGET_PROPERTIES(sqliteConnector PROPERTIES COMPILE_FLAGS -fPIC)

TARGET_LINK_LIBRARIES(sqliteConnector ${jsoncpp_LIBRARY} sqlite3)