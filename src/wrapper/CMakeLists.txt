SET(LINK_SEARCH_START_STATIC ON)
SET(LINK_SEARCH_END_STATIC ON)

FILE(GLOB SRCH2_KAFKA_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
				    "../../thirdparty/kafka/consumer/*.cpp"
					"../../thirdparty/kafka/producer/*.cpp"
					)
					
FILE(GLOB SRCH2_WRAPPER_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp" 
					"../../thirdparty/snappy-1.0.4/*.cc"
					)
					
MESSAGE(STATUS "CMAKE_REQUIRED_LIBRARIES="  ${CMAKE_REQUIRED_LIBRARIES})
MESSAGE(STATUS "Srch2InstantSearch_LIBRARIES="  ${Srch2InstantSearch_LIBRARIES} )

INCLUDE_DIRECTORIES(
	${CMAKE_BINARY_DIR}/include
	${CMAKE_SOURCE_DIR}/src/wrapper/
	${CMAKE_SOURCE_DIR}/src/core/
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/thirdparty/event/include
    ${CMAKE_SOURCE_DIR}/thirdparty/json/jsoncpp-src-0.5.0/include
)

MESSAGE(STATUS "Srch2 source dir="  ${CMAKE_SOURCE_DIR} )
SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-conversion -Wno-sign-compare")		

#--Build KAFKA and other wrapper objects
add_library(KAFKA_OBJECTS OBJECT ${SRCH2_KAFKA_SRC})
add_library(WRAPPER_OBJECTS OBJECT ${SRCH2_WRAPPER_SRC})
