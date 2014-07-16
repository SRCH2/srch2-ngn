EXEC_PROGRAM( ${CMAKE_CXX_COMPILER}
                      ARGS  -dumpversion
                  OUTPUT_VARIABLE gcc_compiler_version)

IF (ANDROID) 
   SET( jsoncpp_LIBRARY ${CMAKE_SOURCE_DIR}/thirdparty/json/jsoncpp-src-0.5.0/android/libs/armeabi-v7a/libjsoncpp.a)
ELSE()
   SET( jsoncpp_LIBRARY ${CMAKE_SOURCE_DIR}/thirdparty/json/jsoncpp-src-0.5.0/libs/linux-gcc-${gcc_compiler_version}/libjson_linux-gcc-${gcc_compiler_version}_libmt.a)
ENDIF()

FILE(GLOB SRCH2_SERVER_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp" "./util/*.cpp" "./util/xmlParser/*.cpp"
                    "../../thirdparty/snappy-1.0.4/*.cc")
LIST(REMOVE_ITEM SRCH2_SERVER_SRC "srch2SearchServer.cpp") 

SET(SRCH2_SERVER_NAME srch2-search-server)

INCLUDE_DIRECTORIES(
	${CMAKE_BINARY_DIR}/include
	${CMAKE_SOURCE_DIR}/src/wrapper/
	${CMAKE_SOURCE_DIR}/src/core/
	${CMAKE_SOURCE_DIR}/src/server/
	${CMAKE_SOURCE_DIR}/src/adapter/
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/thirdparty/event/include
    ${CMAKE_SOURCE_DIR}/thirdparty/event/android/include
    ${CMAKE_SOURCE_DIR}/thirdparty/json/jsoncpp-src-0.5.0/include
    ${CMAKE_SOURCE_DIR}/thirdparty/gperftools/include
)

add_library(SERVER_OBJECTS OBJECT ${SRCH2_SERVER_SRC})

set(GPERFTOOL_LIBS "")
IF(ENABLE_PROFILER)
    set(GPERFTOOL_LIBS ${CMAKE_SOURCE_DIR}/thirdparty/gperftools/lib/libprofiler.a ${CMAKE_SOURCE_DIR}/thirdparty/libunwind/lib/libunwind.a)
ENDIF()

IF(ANDROID)
	ADD_EXECUTABLE(${SRCH2_SERVER_NAME} srch2SearchServer.cpp $<TARGET_OBJECTS:WRAPPER_OBJECTS> $<TARGET_OBJECTS:SERVER_OBJECTS>)
	TARGET_LINK_LIBRARIES(${SRCH2_SERVER_NAME} 
                        ${Srch2InstantSearch_LIBRARIES} 
                        ${jsoncpp_LIBRARY}      ${CMAKE_SOURCE_DIR}/thirdparty/event/android/lib/libevent.a ${CMAKE_SOURCE_DIR}/thirdparty/event/android/lib/libevent_pthreads.a
                        ${CMAKE_REQUIRED_LIBRARIES} ${Boost_LIBRARIES} 
                )
ELSE()

ADD_EXECUTABLE(${SRCH2_SERVER_NAME} srch2SearchServer.cpp $<TARGET_OBJECTS:WRAPPER_OBJECTS> $<TARGET_OBJECTS:SERVER_OBJECTS> $<TARGET_OBJECTS:ADAPTER_OBJECTS>)              
TARGET_LINK_LIBRARIES(${SRCH2_SERVER_NAME}  
                        ${Srch2InstantSearch_LIBRARIES} 
                        ${jsoncpp_LIBRARY}      ${CMAKE_SOURCE_DIR}/thirdparty/event/lib/libevent.a ${CMAKE_SOURCE_DIR}/thirdparty/event/lib/libevent_pthreads.a
                        ${Boost_LIBRARIES} ${GPERFTOOL_LIBS} -Wl,-Bdynamic ${CMAKE_REQUIRED_LIBRARIES}                      
                )
                
ENDIF()

ADD_DEPENDENCIES(${SRCH2_SERVER_NAME} srch2_core)
if (ANDROID)
INSTALL(FILES ${CMAKE_BINARY_DIR}/bin/${SRCH2_SERVER_NAME} DESTINATION bin RENAME "srch2-engine"  PERMISSIONS OWNER_READ OWNER_EXECUTE)
else()
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${SRCH2_SERVER_NAME} DESTINATION bin RENAME "srch2-engine"  PERMISSIONS OWNER_READ OWNER_EXECUTE)
endif()
#SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/../search-server)
