INCLUDE_DIRECTORIES(
    ../../src/core
    ../../include
    ${Boost_INCLUDE_DIRS}
)

SET(UTILITY_LIBS ${Srch2InstantSearch_LIBRARIES} ${Boost_LIBRARIES} ${CMAKE_REQUIRED_LIBRARIES})
ADD_EXECUTABLE(DictionaryBuilder src/DictionaryBuilder.cpp)
ADD_DEPENDENCIES(DictionaryBuilder srch2_core) 
TARGET_LINK_LIBRARIES(DictionaryBuilder ${UTILITY_LIBS} )
