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
    ${CMAKE_SOURCE_DIR}/../thirdparty/mongo-cxx-driver/mongo-cxx-driver/build/use-system-boost/srch2/include/
    ${CMAKE_SOURCE_DIR}/../src/adapter/
)

ADD_LIBRARY(mongodbConnector SHARED ${MONGO_DB_CONNECTOR_SRC} )

set(MONGO_VERSION_MAJOR 1)
set(MONGO_VERSION_MINOR 0)
set(MONGO_VERSION_PATCH 0)
set(MONGO_VERSION_STRING ${MONGO_VERSION_MAJOR}.${MONGO_VERSION_MINOR}.${MONGO_VERSION_PATCH})

set_target_properties(mongodbConnector PROPERTIES VERSION ${MONGO_VERSION_STRING}
                                          SOVERSION ${MONGO_VERSION_MAJOR})

SET_TARGET_PROPERTIES(mongodbConnector PROPERTIES COMPILE_FLAGS -fPIC)

set(MONGO_CLIENT_LIBRARY ${CMAKE_SOURCE_DIR}/../thirdparty/mongo-cxx-driver/mongo-cxx-driver/build/use-system-boost/libmongoclient.a)

SET(Boost_USE_STATIC_LIBS ON)
SET(Boost_USE_MULTITHREAD ON)
FIND_PACKAGE( Boost COMPONENTS program_options system filesystem thread REQUIRED )

TARGET_LINK_LIBRARIES(mongodbConnector ${Boost_LIBRARIES}  ${MONGO_CLIENT_LIBRARY} -pthread )


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

SET( jsoncpp_LIBRARY ${CMAKE_SOURCE_DIR}/../thirdparty/json/jsoncpp-src-0.5.0/libs/linux-gcc-${gcc_compiler_version}/libjson_linux-gcc-${gcc_compiler_version}_libmt.so)
ADD_LIBRARY(sqliteConnector SHARED ${SQLITE_CONNECTOR_SRC} )

SET_TARGET_PROPERTIES(sqliteConnector PROPERTIES COMPILE_FLAGS -fPIC)

TARGET_LINK_LIBRARIES(sqliteConnector ${jsoncpp_LIBRARY} sqlite3 ${Boost_LIBRARIES})

