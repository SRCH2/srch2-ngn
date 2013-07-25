INCLUDE_DIRECTORIES(
	${CMAKE_BINARY_DIR}/include
	${CMAKE_SOURCE_DIR}/src/
	${CMAKE_SOURCE_DIR}/src/core/
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/thirdparty/event/include
    ${CMAKE_SOURCE_DIR}/thirdparty/json/jsoncpp-src-0.5.0/include
)

EXEC_PROGRAM( ${CMAKE_CXX_COMPILER}
                      ARGS  -dumpversion
                  OUTPUT_VARIABLE gcc_compiler_version)
SET( jsoncpp_LIBRARY ${CMAKE_SOURCE_DIR}/thirdparty/json/jsoncpp-src-0.5.0/libs/linux-gcc-${gcc_compiler_version}/libjson_linux-gcc-${gcc_compiler_version}_libmt.a)


ADD_EXECUTABLE(WrapperCreateIndexFromFile_Test WrapperCreateIndexFromFile_Test.cpp $<TARGET_OBJECTS:KAFKA_OBJECTS> $<TARGET_OBJECTS:WRAPPER_OBJECTS>)
TARGET_LINK_LIBRARIES(WrapperCreateIndexFromFile_Test
						${Srch2InstantSearch_LIBRARIES} 
						${jsoncpp_LIBRARY}	${CMAKE_SOURCE_DIR}/thirdparty/event/lib/libevent.a 
						${Boost_LIBRARIES} ${CMAKE_REQUIRED_LIBRARIES} ${CMAKE_SOURCE_DIR}/thirdparty/gperftools/lib/libprofiler.a -lunwind
					)

ADD_DEPENDENCIES(WrapperCreateIndexFromFile_Test srch2_core)
