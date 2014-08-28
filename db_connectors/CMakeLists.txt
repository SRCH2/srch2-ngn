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

#Find the platform information.
SET(MAC_OS OFF CACHE BOOL "")
SET(LINUX_OS OFF CACHE BOOL "")
IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    MESSAGE(STATUS "PLATFORM = MAC OS")
    SET(MAC_OS ON)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    MESSAGE(STATUS "PLATFORM = LINUX")
    SET(LINUX_OS ON)
ELSE()
    MESSAGE(STATUS "PLATFORM = UNKNOWN")
ENDIF()
#
#  1.  MongoDB connector 
#
IF(NOT ANDROID)
    FILE(GLOB_RECURSE MONGO_DB_CONNECTOR_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "mongo/*.cpp")

    MESSAGE(STATUS "This is SOURCE dir " ${CMAKE_SOURCE_DIR})

    IF(MAC_OS)
        SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libstdc++")	    
        INCLUDE_DIRECTORIES(
            ${CMAKE_SOURCE_DIR}/../thirdparty/mongo-cxx-driver/mongo-cxx-driver/build/osx-version-min_10.7/use-system-boost/srch2/include/
            ${CMAKE_SOURCE_DIR}/../src/adapter/
        )
    ELSE()
        INCLUDE_DIRECTORIES(
            ${CMAKE_SOURCE_DIR}/../thirdparty/mongo-cxx-driver/mongo-cxx-driver/build/use-system-boost/srch2/include/
            ${CMAKE_SOURCE_DIR}/../src/adapter/
        )
    ENDIF()

    ADD_LIBRARY(mongodbConnector SHARED ${MONGO_DB_CONNECTOR_SRC} )

    set(MONGO_VERSION_MAJOR 1)
    set(MONGO_VERSION_MINOR 0)
    set(MONGO_VERSION_PATCH 0)
    set(MONGO_VERSION_STRING ${MONGO_VERSION_MAJOR}.${MONGO_VERSION_MINOR}.${MONGO_VERSION_PATCH})

    set_target_properties(mongodbConnector PROPERTIES VERSION ${MONGO_VERSION_STRING}
                                          SOVERSION ${MONGO_VERSION_MAJOR})

    SET_TARGET_PROPERTIES(mongodbConnector PROPERTIES COMPILE_FLAGS -fPIC)

    IF(MAC_OS)
        set(MONGO_CLIENT_LIBRARY ${CMAKE_SOURCE_DIR}/../thirdparty/mongo-cxx-driver/mongo-cxx-driver/build/osx-version-min_10.7/use-system-boost/libmongoclient.a)
    ELSE()
        set(MONGO_CLIENT_LIBRARY ${CMAKE_SOURCE_DIR}/../thirdparty/mongo-cxx-driver/mongo-cxx-driver/build/use-system-boost/libmongoclient.a)
    ENDIF()

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

MESSAGE(STATUS "Boost_VERSION=" ${Boost_VERSION})
MESSAGE(STATUS "Boost_LIBRARIES=" ${Boost_LIBRARIES})
MESSAGE(STATUS "Boost_INCLUDE_DIRS=" ${Boost_INCLUDE_DIRS})

EXEC_PROGRAM( ${CMAKE_CXX_COMPILER}
                      ARGS  -dumpversion
                  OUTPUT_VARIABLE gcc_compiler_version)


FILE(GLOB_RECURSE SQLITE_CONNECTOR_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "sqlite/*.cpp")
					
INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/../src/adapter/
    ${CMAKE_SOURCE_DIR}/../thirdparty/json/jsoncpp-src/include
    ${Boost_INCLUDE_DIRS}
)

IF (ANDROID) 
    SET( jsoncpp_LIBRARY ${CMAKE_SOURCE_DIR}/../thirdparty/json/jsoncpp-src/android/libs/armeabi-v7a/libjsoncpp.a)
    SET(CMAKE_REQUIRED_LIBRARIES stdc++ crypto ${CMAKE_REQUIRED_LIBRARIES})
ELSE()
    SET( jsoncpp_LIBRARY ${CMAKE_SOURCE_DIR}/../thirdparty/json/jsoncpp-src/build/libjsoncpp.a)
    IF (LINUX_OS)
        SET(CMAKE_REQUIRED_LIBRARIES dl pthread stdc++ rt z ${CMAKE_REQUIRED_LIBRARIES})
    ELSE()
	SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libstdc++")
        SET(CMAKE_REQUIRED_LIBRARIES pthread stdc++ ${CMAKE_REQUIRED_LIBRARIES})
    ENDIF()
ENDIF()

ADD_LIBRARY(sqliteConnector SHARED ${SQLITE_CONNECTOR_SRC} )

set(SQLITE_VERSION_MAJOR 1)
set(SQLITE_VERSION_MINOR 0)
set(SQLITE_VERSION_PATCH 0)
set(SQLITE_VERSION_STRING ${SQLITE_VERSION_MAJOR}.${SQLITE_VERSION_MINOR}.${SQLITE_VERSION_PATCH})

set_target_properties(sqliteConnector PROPERTIES VERSION ${SQLITE_VERSION_STRING}
                                          SOVERSION ${SQLITE_VERSION_MAJOR})

SET_TARGET_PROPERTIES(sqliteConnector PROPERTIES COMPILE_FLAGS -fPIC)

TARGET_LINK_LIBRARIES(sqliteConnector ${jsoncpp_LIBRARY} ${CMAKE_REQUIRED_LIBRARIES} ${Boost_LIBRARIES} sqlite3)

