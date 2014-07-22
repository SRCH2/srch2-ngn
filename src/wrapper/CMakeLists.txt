SET(LINK_SEARCH_START_STATIC ON)
SET(LINK_SEARCH_END_STATIC ON)

FILE(GLOB SRCH2_WRAPPER_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp")
					
INCLUDE_DIRECTORIES(
	${CMAKE_BINARY_DIR}/include
	${CMAKE_SOURCE_DIR}/src/wrapper/
	${CMAKE_SOURCE_DIR}/src/core/
	${CMAKE_SOURCE_DIR}/src/server/
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/thirdparty/event/include
    ${CMAKE_SOURCE_DIR}/thirdparty/event/android/include
    ${CMAKE_SOURCE_DIR}/thirdparty/json/jsoncpp-src-0.5.0/include
)

SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-conversion -Wno-sign-compare")		

add_library(WRAPPER_OBJECTS OBJECT ${SRCH2_WRAPPER_SRC})
