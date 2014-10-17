#  How to use this cmake file.
# > mdkir build
# > cd build
# > cmake -DSTATIC_LINK_BOOST=0 ..
# > make
# 
# To build on android, please use
# > cmake -DANDROID=1 ..


cmake_minimum_required(VERSION 2.8)

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
    FILE(GLOB_RECURSE MONGO_DB_CONNECTOR_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "mongo/*.cpp" "util/*.cpp" "../src/core/util/Logger.cpp")

    MESSAGE(STATUS "This is SOURCE dir " ${CMAKE_SOURCE_DIR})

    IF(MAC_OS)
        SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libstdc++")	    
        INCLUDE_DIRECTORIES(
            ${CMAKE_SOURCE_DIR}/../thirdparty/mongo-cxx-driver/mongo-cxx-driver/build/osx-version-min_10.7/use-system-boost/srch2/include/
            ${CMAKE_SOURCE_DIR}/../src/adapter/
            ${CMAKE_SOURCE_DIR}/util/
        )
    ELSE()
        INCLUDE_DIRECTORIES(
            ${CMAKE_SOURCE_DIR}/../thirdparty/mongo-cxx-driver/mongo-cxx-driver/build/use-system-boost/srch2/include/
            ${CMAKE_SOURCE_DIR}/../src/adapter/
            ${CMAKE_SOURCE_DIR}/util/
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
    
    SET(Boost_USE_STATIC_LIBS ON)
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

EXEC_PROGRAM( ${CMAKE_CXX_COMPILER}
                      ARGS  -dumpversion
                  OUTPUT_VARIABLE gcc_compiler_version)


FILE(GLOB_RECURSE SQLITE_CONNECTOR_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "sqlite/*.cpp" "util/*.cpp" "../src/core/util/Logger.cpp")
					
INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/sqlite/
    ${CMAKE_SOURCE_DIR}/../src/adapter/
    ${CMAKE_SOURCE_DIR}/../thirdparty/json/jsoncpp-src/include
    ${CMAKE_SOURCE_DIR}/util/
)

IF (ANDROID) 
    FIND_LIBRARY(log_LIBRARY NAMES log PATHS ${sys_LIBRARY_DIRS})
    
    MESSAGE(STATUS "liblog = ${log_LIBRARY}")

    SET( jsoncpp_LIBRARY ${CMAKE_SOURCE_DIR}/../thirdparty/json/jsoncpp-src/android/libs/armeabi-v7a/libjsoncpp.a)
    SET(CMAKE_REQUIRED_LIBRARIES stdc++ ${CMAKE_REQUIRED_LIBRARIES} ${log_LIBRARY})
    LINK_DIRECTORIES("${sys_LIBRARY_DIRS}")
ELSE()
    SET( jsoncpp_LIBRARY ${CMAKE_SOURCE_DIR}/../thirdparty/json/jsoncpp-src/build/libjsoncpp.a)
    IF (MAC_OS)
        SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libstdc++")
    ENDIF()

    SET(CMAKE_REQUIRED_LIBRARIES pthread stdc++ ${CMAKE_REQUIRED_LIBRARIES})
    
ENDIF()

ADD_LIBRARY(sqliteConnector SHARED ${SQLITE_CONNECTOR_SRC} )

set(SQLITE_VERSION_MAJOR 1)
set(SQLITE_VERSION_MINOR 0)
set(SQLITE_VERSION_PATCH 0)
set(SQLITE_VERSION_STRING ${SQLITE_VERSION_MAJOR}.${SQLITE_VERSION_MINOR}.${SQLITE_VERSION_PATCH})

set_target_properties(sqliteConnector PROPERTIES VERSION ${SQLITE_VERSION_STRING}
                                          SOVERSION ${SQLITE_VERSION_MAJOR})

SET_TARGET_PROPERTIES(sqliteConnector PROPERTIES COMPILE_FLAGS -fPIC)

IF (ANDROID) 
    TARGET_LINK_LIBRARIES(sqliteConnector ${jsoncpp_LIBRARY} ${CMAKE_REQUIRED_LIBRARIES} libsqlite.so)
ELSE()
    TARGET_LINK_LIBRARIES(sqliteConnector ${jsoncpp_LIBRARY} ${CMAKE_REQUIRED_LIBRARIES} sqlite3)
ENDIF()


#
#  3.  MySQL connector 
#

IF(NOT ANDROID)
    FILE(GLOB_RECURSE MYSQL_CONNECTOR_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "mysql/*.cpp" "util/*.cpp" "../src/core/util/Logger.cpp")

    IF(MAC_OS)
        SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libstdc++")	
        
        #The MySQL default installation path on MAC is different from the path on Linux 
        INCLUDE_DIRECTORIES(
            ${CMAKE_SOURCE_DIR}/mysql/
            ${CMAKE_SOURCE_DIR}/../src/adapter/
            ${CMAKE_SOURCE_DIR}/../thirdparty/json/jsoncpp-src/include
            ${CMAKE_SOURCE_DIR}/util/
            ${CMAKE_SOURCE_DIR}/../thirdparty/mysql-connector-c++/mysql-replication-listener/include
            ${CMAKE_SOURCE_DIR}/../thirdparty/mysql-connector-c++/mysql-connector-c++/build/include
            /usr/local/mysql/include/
    )
    ELSE() 
        INCLUDE_DIRECTORIES(
            ${CMAKE_SOURCE_DIR}/mysql/
            ${CMAKE_SOURCE_DIR}/../src/adapter/
            ${CMAKE_SOURCE_DIR}/../thirdparty/json/jsoncpp-src/include
            ${CMAKE_SOURCE_DIR}/util/
            ${CMAKE_SOURCE_DIR}/../thirdparty/mysql-connector-c++/mysql-replication-listener/include
            ${CMAKE_SOURCE_DIR}/../thirdparty/mysql-connector-c++/mysql-connector-c++/build/include
            /usr/include/mysql/
        )
    ENDIF()

    SET( jsoncpp_LIBRARY ${CMAKE_SOURCE_DIR}/../thirdparty/json/jsoncpp-src/build/libjsoncpp.a)

    #MYSQL_CONNECTOR_LIBRARY and MYSQL_LISTENER_LIBRARY have to be both .a (static libs) or .so (or dynamic libs). We choose to use .a
    SET(MYSQL_LISTENER_LIBRARY ${CMAKE_SOURCE_DIR}/../thirdparty/mysql-connector-c++/mysql-replication-listener/build/lib/libreplication.a)
    SET(MYSQL_CONNECTOR_LIBRARY ${CMAKE_SOURCE_DIR}/../thirdparty/mysql-connector-c++/mysql-connector-c++/build/lib/libmysqlcppconn-static.a)

    ADD_LIBRARY(mysqlConnector SHARED ${MYSQL_CONNECTOR_SRC} )

    set(MYSQL_VERSION_MAJOR 1)
    set(MYSQL_VERSION_MINOR 0)
    set(MYSQL_VERSION_PATCH 0)
    set(MYSQL_VERSION_STRING ${MYSQL_VERSION_MAJOR}.${MYSQL_VERSION_MINOR}.${MYSQL_VERSION_PATCH})

    set_target_properties(mysqlConnector PROPERTIES VERSION ${MYSQL_VERSION_STRING}
                                          SOVERSION ${MYSQL_VERSION_MAJOR})

    SET_TARGET_PROPERTIES(mysqlConnector PROPERTIES COMPILE_FLAGS -fPIC)

    IF(MAC_OS)
        TARGET_LINK_LIBRARIES(mysqlConnector  ${MYSQL_LISTENER_LIBRARY} /usr/local/mysql/lib/libmysqlclient.a  ${MYSQL_CONNECTOR_LIBRARY} ${jsoncpp_LIBRARY} )
    ELSE()
        TARGET_LINK_LIBRARIES(mysqlConnector  ${MYSQL_LISTENER_LIBRARY} mysqlclient  ${MYSQL_CONNECTOR_LIBRARY} ${jsoncpp_LIBRARY} )
    ENDIF()

ENDIF()

#
#  4.  SQLServer connector 
#

IF(NOT ANDROID)
    FILE(GLOB_RECURSE SQLSERVER_CONNECTOR_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "sqlserver/*.cpp" "util/*.cpp" "../src/core/util/Logger.cpp")

    INCLUDE_DIRECTORIES(
        ${CMAKE_SOURCE_DIR}/sqlserver/
        ${CMAKE_SOURCE_DIR}/../src/adapter/
        ${CMAKE_SOURCE_DIR}/../thirdparty/json/jsoncpp-src/include
        ${CMAKE_SOURCE_DIR}/util/
    )
    
    SET( jsoncpp_LIBRARY ${CMAKE_SOURCE_DIR}/../thirdparty/json/jsoncpp-src/build/libjsoncpp.a)
    
    ADD_LIBRARY(sqlserverConnector SHARED ${SQLSERVER_CONNECTOR_SRC} )
    
    set(SQLSERVER_VERSION_MAJOR 1)
    set(SQLSERVER_VERSION_MINOR 0)
    set(SQLSERVER_VERSION_PATCH 0)
    set(SQLSERVER_VERSION_STRING ${SQLSERVER_VERSION_MAJOR}.${SQLSERVER_VERSION_MINOR}.${SQLSERVER_VERSION_PATCH})
    
    set_target_properties(sqlserverConnector PROPERTIES VERSION ${SQLSERVER_VERSION_STRING}
                                          SOVERSION ${SQLSERVER_VERSION_MAJOR})
                                          
    SET_TARGET_PROPERTIES(sqlserverConnector PROPERTIES COMPILE_FLAGS -fPIC)
    
    TARGET_LINK_LIBRARIES(sqlserverConnector ${jsoncpp_LIBRARY} -lodbc)
ENDIF()


