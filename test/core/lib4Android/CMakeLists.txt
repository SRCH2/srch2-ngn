INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/src/core/
    ${CMAKE_BINARY_DIR}/include/
    ${Boost_INCLUDE_DIRS}
)

LINK_DIRECTORIES( /home/jianfeng/software/android-toolchain/user/lib )

SET(IndexIntegration_Source  ${CMAKE_SOURCE_DIR}/test/core/integration/IntegrationTestHelper.cpp)
SET(MapSearchTest_Source  ${CMAKE_SOURCE_DIR}/test/core/integration/MapSearchTestHelper.cpp)
SET(INTEGRATION_TEST_LIBS ${Srch2InstantSearch_LIBRARIES} ${Boost_LIBRARIES} ${CMAKE_REQUIRED_LIBRARIES})

ADD_LIBRARY(srch2sdk SHARED Srch2Android.cpp SearchSDK.cpp Scalability_Test.cpp ${IndexIntegration_Source} ${MapSearchTest_Source} )
SET_TARGET_PROPERTIES(srch2sdk PROPERTIES COMPILE_FLAGS -fPIC)
TARGET_LINK_LIBRARIES(srch2sdk ${INTEGRATION_TEST_LIBS} log )	
