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

