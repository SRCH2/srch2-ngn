# define the sources for lib-srch2. 
#Inside the source directory, to make a header file invisible to build environment, name it in "_${name}.h" format. For example, this applies to test headers like _TrieTest.h

#CHENLI: Disable M1 code for now
FILE(GLOB SRCH2_LIB_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp" "index/*.cpp" "ranking/*.cpp" "operation/*.cpp" "record/*.cpp" "util/*.cpp" "analyzer/*.cpp" "query/*.cpp" "license/*.cpp" "geo/*.cpp")
 
# additional includes
INCLUDE_DIRECTORIES(. "${CMAKE_BINARY_DIR}/include" ${Boost_INCLUDE_DIRS})
LINK_DIRECTORIES( /usr/lib/
				  /usr/lib/x86_64-linux-gnu/
				  /usr/lib64/)

#CHENLI: generating a shared library if needed.
IF (BUILD_STATIC)
  ADD_LIBRARY(srch2_core ${SRCH2_LIB_SRC} )
ELSE (BUILD_STATIC)
  ADD_LIBRARY(srch2_core SHARED ${SRCH2_LIB_SRC} )
ENDIF()

IF(ENABLE_PROFILER)
    ADD_DEFINITIONS(-DENABLE_PROFILER)
ENDIF()

SET_TARGET_PROPERTIES(srch2_core PROPERTIES COMPILE_FLAGS -fPIC)
TARGET_LINK_LIBRARIES(srch2_core ${CMAKE_REQUIRED_LIBRARIES} ${Boost_LIBRARIES})				

# install
#INSTALL(TARGETS srch2_core DESTINATION lib)


