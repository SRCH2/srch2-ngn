INCLUDE_DIRECTORIES(
	${CMAKE_BINARY_DIR}/include
	${CMAKE_SOURCE_DIR}/src/
	${CMAKE_SOURCE_DIR}/src/wrapper/
	${CMAKE_SOURCE_DIR}/src/core/
	${CMAKE_SOURCE_DIR}/src/server/
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/thirdparty/event/include
    ${CMAKE_SOURCE_DIR}/thirdparty/json/jsoncpp-src-0.5.0/include
    ${CMAKE_SOURCE_DIR}/thirdparty/mongo-cxx-driver-v2.4/src/
)

EXEC_PROGRAM( ${CMAKE_CXX_COMPILER}
                      ARGS  -dumpversion
                  OUTPUT_VARIABLE gcc_compiler_version)
set(MONGO_CLIENT_LIBRARY ${CMAKE_SOURCE_DIR}/thirdparty/mongo-cxx-driver-v2.4/libmongoclient.a)
                  
SET( jsoncpp_LIBRARY ${CMAKE_SOURCE_DIR}/thirdparty/json/jsoncpp-src-0.5.0/libs/linux-gcc-${gcc_compiler_version}/libjson_linux-gcc-${gcc_compiler_version}_libmt.a)


ADD_EXECUTABLE(ConfigManager_Test ConfigManager_Test.cpp $<TARGET_OBJECTS:WRAPPER_OBJECTS> $<TARGET_OBJECTS:SERVER_OBJECTS>)
TARGET_LINK_LIBRARIES(ConfigManager_Test
                        ${Srch2InstantSearch_LIBRARIES} 
                        ${jsoncpp_LIBRARY}  ${CMAKE_SOURCE_DIR}/thirdparty/event/lib/libevent.a 
                        ${MONGO_CLIENT_LIBRARY} ${Boost_LIBRARIES} ${CMAKE_REQUIRED_LIBRARIES}  ${GPERFTOOL_LIBS}
                    )                    
ADD_DEPENDENCIES(ConfigManager_Test srch2_core)
LIST(APPEND UNIT_TESTS ConfigManager_Test)


ADD_EXECUTABLE(JSONValueObjectToRecord_Test JSONValueObjectToRecord_Test/JSONValueObjectToRecord_Test.cpp $<TARGET_OBJECTS:WRAPPER_OBJECTS> $<TARGET_OBJECTS:SERVER_OBJECTS>)
TARGET_LINK_LIBRARIES(JSONValueObjectToRecord_Test
                        ${Srch2InstantSearch_LIBRARIES} 
                        ${jsoncpp_LIBRARY}  ${CMAKE_SOURCE_DIR}/thirdparty/event/lib/libevent.a 
                        ${MONGO_CLIENT_LIBRARY} ${Boost_LIBRARIES} ${CMAKE_REQUIRED_LIBRARIES}  ${GPERFTOOL_LIBS}
                    )    
ADD_DEPENDENCIES(JSONValueObjectToRecord_Test srch2_core)
LIST(APPEND UNIT_TESTS JSONValueObjectToRecord_Test)