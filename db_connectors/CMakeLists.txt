#  How to use this cmake file.
# > mdkir build
# > cd build
# > cmake -DRELEASE_BUILD=0 ..
# > make
# 
# To build on android, please use
# > cmake -DANDROID=1 ..


cmake_minimum_required(VERSION 2.8)
SET(RELEASE_BUILD ON CACHE BOOL "")
SET(ANDROID OFF CACHE BOOL "")
#
#  1.  MongoDB connector 
#
IF(NOT ANDROID)
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
    if (RELEASE_BUILD)
        SET(Boost_USE_STATIC_LIBS ON)
    endif()
    SET(Boost_USE_MULTITHREAD ON)
    FIND_PACKAGE( Boost COMPONENTS program_options system filesystem thread REQUIRED )
    
    TARGET_LINK_LIBRARIES(mongodbConnector ${Boost_LIBRARIES}  ${MONGO_CLIENT_LIBRARY} -pthread )
ENDIF()

#
#  2.  SQLite connector 
#

IF (ANDROID)
    SET(sys_LIBRARY_DIRS "${ANDROID_TOOLCHAIN_ROOT}/user/lib")
ELSE()
    SET(sys_LIBRARY_DIRS "/usr/lib64/" "/lib/" "/lib64/")
ENDIF()

SET(Boost_USE_MULTITHREAD ON)
    FIND_PACKAGE( Boost COMPONENTS serialization program_options regex system thread filesystem REQUIRED)

MESSAGE(STATUS "Boost_VERSION=" ${Boost_VERSION})
MESSAGE(STATUS "Boost_LIBRARIES=" ${Boost_LIBRARIES})
MESSAGE(STATUS "Boost_INCLUDE_DIRS=" ${Boost_INCLUDE_DIRS})

EXEC_PROGRAM( ${CMAKE_CXX_COMPILER}
                      ARGS  -dumpversion
                  OUTPUT_VARIABLE gcc_compiler_version)


FILE(GLOB_RECURSE SQLITE_CONNECTOR_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "sqlite/*.cpp")
					
INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/sqlite/
    ${CMAKE_SOURCE_DIR}/../src/adapter/
    ${CMAKE_SOURCE_DIR}/../thirdparty/json/jsoncpp-src-0.5.0/include
    ${Boost_INCLUDE_DIRS}
)

IF (ANDROID) 
    FIND_LIBRARY(log_LIBRARY NAMES log PATHS ${sys_LIBRARY_DIRS})
    FIND_LIBRARY(dl_LIBRARY NAMES dl PATHS ${sys_LIBRARY_DIRS})
    FIND_LIBRARY(z_LIBRARY NAMES z PATHS ${sys_LIBRARY_DIRS})

    MESSAGE(STATUS "liblog = ${log_LIBRARY}")
    MESSAGE(STATUS "libdl = ${dl_LIBRARY}")
    MESSAGE(STATUS "libz = ${z_LIBRARY}")
   
    SET( jsoncpp_LIBRARY ${CMAKE_SOURCE_DIR}/../thirdparty/json/jsoncpp-src-0.5.0/android/libs/armeabi-v7a/libjsoncpp.a)
    SET(CMAKE_REQUIRED_LIBRARIES stdc++ crypto ${CMAKE_REQUIRED_LIBRARIES} ${dl_LIBRARY} ${z_LIBRARY} ${log_LIBRARY})
    LINK_DIRECTORIES("${sys_LIBRARY_DIRS}")
ELSE()
    SET( jsoncpp_LIBRARY ${CMAKE_SOURCE_DIR}/../thirdparty/json/jsoncpp-src-0.5.0/libs/linux-gcc-${gcc_compiler_version}/libjson_linux-gcc-${gcc_compiler_version}_libmt.a)
    IF (LINUX_OS)
        SET(CMAKE_REQUIRED_LIBRARIES dl pthread stdc++ rt z ${CMAKE_REQUIRED_LIBRARIES})
    ELSE()
        SET(CMAKE_REQUIRED_LIBRARIES pthread stdc++ ${CMAKE_REQUIRED_LIBRARIES})
    ENDIF()
    
    FIND_PACKAGE( Boost COMPONENTS filesystem REQUIRED )
ENDIF()

#SET( jsoncpp_LIBRARY ${CMAKE_SOURCE_DIR}/../thirdparty/json/jsoncpp-src/build/libjsoncpp.a)
ADD_LIBRARY(sqliteConnector SHARED ${SQLITE_CONNECTOR_SRC} )

set(SQLITE_VERSION_MAJOR 1)
set(SQLITE_VERSION_MINOR 0)
set(SQLITE_VERSION_PATCH 0)
set(SQLITE_VERSION_STRING ${SQLITE_VERSION_MAJOR}.${SQLITE_VERSION_MINOR}.${SQLITE_VERSION_PATCH})

set_target_properties(sqliteConnector PROPERTIES VERSION ${SQLITE_VERSION_STRING}
                                          SOVERSION ${SQLITE_VERSION_MAJOR})

SET_TARGET_PROPERTIES(sqliteConnector PROPERTIES COMPILE_FLAGS -fPIC)



TARGET_LINK_LIBRARIES(sqliteConnector ${jsoncpp_LIBRARY} ${CMAKE_SOURCE_DIR}/../thirdparty/event/android/lib/libevent.a ${CMAKE_SOURCE_DIR}/../thirdparty/event/android/lib/libevent_pthreads.a ${CMAKE_REQUIRED_LIBRARIES} ${Boost_LIBRARIES} sqlite3)

