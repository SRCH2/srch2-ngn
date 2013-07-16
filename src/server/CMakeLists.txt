EXEC_PROGRAM( ${CMAKE_CXX_COMPILER}
                      ARGS  -dumpversion
                  OUTPUT_VARIABLE gcc_compiler_version)
SET( jsoncpp_LIBRARY ${CMAKE_SOURCE_DIR}/thirdparty/json/jsoncpp-src-0.5.0/libs/linux-gcc-${gcc_compiler_version}/libjson_linux-gcc-${gcc_compiler_version}_libmt.a)

SET(SRCH2_SERVER_NAME srch2-search-server)

INCLUDE_DIRECTORIES(
	${CMAKE_BINARY_DIR}/include
	${CMAKE_SOURCE_DIR}/src/wrapper/
	${CMAKE_SOURCE_DIR}/src/core/
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/thirdparty/event/include
    ${CMAKE_SOURCE_DIR}/thirdparty/json/jsoncpp-src-0.5.0/include
)

ADD_EXECUTABLE(${SRCH2_SERVER_NAME} srch2SearchServer.cpp $<TARGET_OBJECTS:KAFKA_OBJECTS> $<TARGET_OBJECTS:WRAPPER_OBJECTS>)
TARGET_LINK_LIBRARIES(${SRCH2_SERVER_NAME} 
                        ${Srch2InstantSearch_LIBRARIES} 
                        ${jsoncpp_LIBRARY}      ${CMAKE_SOURCE_DIR}/thirdparty/event/lib/libevent.a 
                        ${Boost_LIBRARIES} ${CMAKE_REQUIRED_LIBRARIES}
                )

ADD_DEPENDENCIES(${SRCH2_SERVER_NAME} srch2_core)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${SRCH2_SERVER_NAME} DESTINATION bin RENAME "srch2-engine"  PERMISSIONS OWNER_READ OWNER_EXECUTE)
#SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/../search-server)
