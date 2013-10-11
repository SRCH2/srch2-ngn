# define the sources for lib-srch2. 
#Inside the source directory, to make a header file invisible to build environment, name it in "_${name}.h" format. For example, this applies to test headers like _TrieTest.h

#CHENLI: Disable M1 code for now
FILE(GLOB SRCH2_LIB_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp" "index/*.cpp" "ranking/*.cpp" "operation/*.cpp" "postprocessing/*.cpp" "record/*.cpp" "util/*.cpp" "analyzer/*.cpp" "query/*.cpp" "license/*.cpp" "geo/*.cpp" "../../thirdparty/snappy-1.0.4/*.cc" "serialization/*.cpp")

IF(ANDROID)
    FILE(GLOB JNI_FILE RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "../jni/*.cpp")
    SET(SRCH2_LIB_SRC ${SRCH2_LIB_SRC} ${JNI_FILE})
ENDIF()

# additional includes
INCLUDE_DIRECTORIES(. "${CMAKE_BINARY_DIR}/include" ${Boost_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR})

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
TARGET_LINK_LIBRARIES(srch2_core ${Boost_LIBRARIES} ${CMAKE_REQUIRED_LIBRARIES})				

# install
#INSTALL(TARGETS srch2_core DESTINATION lib)


