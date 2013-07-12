INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/src/core/
    ${CMAKE_BINARY_DIR}/include/
    ${Boost_INCLUDE_DIRS}
)

LINK_DIRECTORIES( /home/jianfeng/software/android-toolchain/user/lib )

SET(IndexIntegration_Source  ${CMAKE_SOURCE_DIR}/test/core/integration/IntegrationTestHelper.h)
SET(MapSearchTest_Source  ${CMAKE_SOURCE_DIR}/test/core/integration/MapSearchTestHelper.h)
SET(INTEGRATION_TEST_LIBS ${Srch2InstantSearch_LIBRARIES} ${Boost_LIBRARIES} ${CMAKE_REQUIRED_LIBRARIES})

ADD_LIBRARY(srch2scaleTest SHARED Scalability_Test.cpp ${IndexIntegration_Source} ${MapSearchTest_Source})
SET_TARGET_PROPERTIES(srch2scaleTest PROPERTIES COMPILE_FLAGS -fPIC)
TARGET_LINK_LIBRARIES(srch2scaleTest ${INTEGRATION_TEST_LIBS} log )	
