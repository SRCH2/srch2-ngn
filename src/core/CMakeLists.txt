# define the sources for lib-bimaple. 
#Inside the source directory, to make a header file invisible to build environment, name it in "_${name}.h" format. For example, this applies to test headers like _TrieTest.h

#CHENLI: Disable M1 code for now
FILE(GLOB BIMAPLE_LIB_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp" "index/*.cpp" "ranking/*.cpp" "operation/*.cpp" "record/*.cpp" "util/*.cpp" "analyzer/*.cpp" "query/*.cpp" "license/*.cpp" "geo/*.cpp")
#FILE(GLOB BIMAPLE_LIB_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp" "index/*.cpp" "ranking/*.cpp" "operation/*.cpp" "record/*.cpp" "util/*.cpp" "query/*.cpp" "license/*.cpp")
 
# additional includes
INCLUDE_DIRECTORIES(. "${CMAKE_BINARY_DIR}/include" ${Boost_INCLUDE_DIRS})
LINK_DIRECTORIES( /usr/lib/
				  /usr/lib/x86_64-linux-gnu/
				  /usr/lib64/)

#CHENLI: generating a shared library if needed.
#ADD_LIBRARY(bimaple_instantsearch ${BIMAPLE_LIB_SRC} )
#ADD_LIBRARY(bimaple_instantsearch SHARED ${BIMAPLE_LIB_SRC} )
IF (BUILD_STATIC)
  ADD_LIBRARY(bimaple_instantsearch ${BIMAPLE_LIB_SRC} )
ELSE (BUILD_STATIC)
  ADD_LIBRARY(bimaple_instantsearch SHARED ${BIMAPLE_LIB_SRC} )
ENDIF()

SET_TARGET_PROPERTIES(bimaple_instantsearch PROPERTIES COMPILE_FLAGS -fPIC)
TARGET_LINK_LIBRARIES(bimaple_instantsearch ${CMAKE_REQUIRED_LIBRARIES})				

ADD_CUSTOM_TARGET(bimaple_lib ALL DEPENDS bimaple_instantsearch)

# install
INSTALL(TARGETS bimaple_instantsearch DESTINATION lib)


