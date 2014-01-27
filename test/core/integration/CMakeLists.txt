INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/src/core/
    ${CMAKE_BINARY_DIR}/include/
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
)

set(GPERFTOOL_LIBS "")
IF(ENABLE_PROFILER)
    set(GPERFTOOL_LIBS ${CMAKE_SOURCE_DIR}/thirdparty/gperftools/lib/libprofiler.a ${CMAKE_SOURCE_DIR}/thirdparty/libunwind/lib/libunwind.a)
ENDIF()

SET(IndexIntegration_Source  ${CMAKE_SOURCE_DIR}/test/core/integration/IntegrationTestHelper.h)
SET(MapSearchTest_Source  ${CMAKE_SOURCE_DIR}/test/core/integration/MapSearchTestHelper.h)
SET(INTEGRATION_TEST_LIBS ${Srch2InstantSearch_LIBRARIES} ${Boost_LIBRARIES} ${CMAKE_REQUIRED_LIBRARIES} ${GPERFTOOL_LIBS})

ADD_EXECUTABLE(BuildIndexesForIntegrationTest BuildIndexesForIntegrationTest.cpp ${IndexIntegration_Source} )
TARGET_LINK_LIBRARIES(BuildIndexesForIntegrationTest ${INTEGRATION_TEST_LIBS})
LIST(APPEND INTEGRATION_TESTS BuildIndexesForIntegrationTest)
# CHENLI: deprecated
#ADD_EXECUTABLE(IndexIntegration_Test IndexIntegration_Test.cpp ${IndexIntegration_Source})
#TARGET_LINK_LIBRARIES(IndexIntegration_Test ${INTEGRATION_TEST_LIBS})

#ADD_EXECUTABLE(CacheIntegration_Test CacheIntegration_Test.cpp ${IndexIntegration_Source})
#TARGET_LINK_LIBRARIES(CacheIntegration_Test ${INTEGRATION_TEST_LIBS})
#LIST(APPEND INTEGRATION_TESTS CacheIntegration_Test)

ADD_EXECUTABLE(QueryStress_Test QueryStress_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(QueryStress_Test ${INTEGRATION_TEST_LIBS})
LIST(APPEND INTEGRATION_TESTS QueryStress_Test)

ADD_EXECUTABLE(DoubleQueryStress_Test DoubleQueryStress_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(DoubleQueryStress_Test  ${INTEGRATION_TEST_LIBS})
LIST(APPEND INTEGRATION_TESTS DoubleQueryStress_Test)

ADD_EXECUTABLE(EditDistance_Test EditDistance_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(EditDistance_Test ${INTEGRATION_TEST_LIBS})
LIST(APPEND INTEGRATION_TESTS EditDistance_Test)
 
ADD_EXECUTABLE(IndexUpdater_Test IndexUpdater_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(IndexUpdater_Test ${INTEGRATION_TEST_LIBS})
LIST(APPEND INTEGRATION_TESTS IndexUpdater_Test)

ADD_EXECUTABLE(MultipleLanguage_Test MultipleLanguage_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(MultipleLanguage_Test ${INTEGRATION_TEST_LIBS})
LIST(APPEND INTEGRATION_TESTS MultipleLanguage_Test)

ADD_EXECUTABLE(MultipleLanguage_2_Test MultipleLanguage_2_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(MultipleLanguage_2_Test ${INTEGRATION_TEST_LIBS})
LIST(APPEND INTEGRATION_TESTS MultipleLanguage_2_Test)

# ADD_EXECUTABLE(StemmerIntegration_Test StemmerIntegration_Test.cpp ${IndexIntegration_Source})
# TARGET_LINK_LIBRARIES(StemmerIntegration_Test  ${INTEGRATION_TEST_LIBS})
	
# ADD_EXECUTABLE(NormalizerIntegration_WithoutSerialization_Test NormalizerIntegration_WithoutSerialization_Test.cpp ${IndexIntegration_Source})
# TARGET_LINK_LIBRARIES(NormalizerIntegration_WithoutSerialization_Test  ${INTEGRATION_TEST_LIBS})
	
# ADD_EXECUTABLE(NormalizerIntegration_Test NormalizerIntegration_Test.cpp ${IndexIntegration_Source})
# TARGET_LINK_LIBRARIES(NormalizerIntegration_Test  ${INTEGRATION_TEST_LIBS})

ADD_EXECUTABLE(ThreadSafety_QueryStress_Test ThreadSafety_QueryStress_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(ThreadSafety_QueryStress_Test ${INTEGRATION_TEST_LIBS} ${THREADING_TEST_LIBS})
LIST(APPEND INTEGRATION_TESTS ThreadSafety_QueryStress_Test)

ADD_EXECUTABLE(ThreadSafetyReadWrite_Test ThreadSafetyReadWrite_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(ThreadSafetyReadWrite_Test ${INTEGRATION_TEST_LIBS} ${THREADING_TEST_LIBS})	
LIST(APPEND INTEGRATION_TESTS ThreadSafetyReadWrite_Test)

ADD_EXECUTABLE(KeywordReEncoding_Test KeywordReEncoding_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(KeywordReEncoding_Test ${INTEGRATION_TEST_LIBS} ${THREADING_TEST_LIBS})	
LIST(APPEND INTEGRATION_TESTS KeywordReEncoding_Test)

ADD_EXECUTABLE(Scalability_Test Scalability_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(Scalability_Test ${INTEGRATION_TEST_LIBS} ${THREADING_TEST_LIBS})	
LIST(APPEND INTEGRATION_TESTS Scalability_Test)

ADD_EXECUTABLE(Scoring_Test Scoring_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(Scoring_Test ${INTEGRATION_TEST_LIBS} ${THREADING_TEST_LIBS})	
LIST(APPEND INTEGRATION_TESTS Scoring_Test)

ADD_EXECUTABLE(GetAllResultsQuery_Test GetAllResultsQuery_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(GetAllResultsQuery_Test ${INTEGRATION_TEST_LIBS} ${THREADING_TEST_LIBS})	
LIST(APPEND INTEGRATION_TESTS GetAllResultsQuery_Test)

ADD_EXECUTABLE(Serialization_on_Running_Test Serialization_on_Running_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(Serialization_on_Running_Test ${INTEGRATION_TEST_LIBS} ${THREADING_TEST_LIBS})	
LIST(APPEND INTEGRATION_TESTS Serialization_on_Running_Test)

ADD_EXECUTABLE(LargeInsertionAfterCommit_Test LargeInsertionAfterCommit_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(LargeInsertionAfterCommit_Test ${INTEGRATION_TEST_LIBS} ${THREADING_TEST_LIBS})	
LIST(APPEND INTEGRATION_TESTS LargeInsertionAfterCommit_Test)

ADD_EXECUTABLE(RankingAfterNewRecordInserted_Test RankingAfterNewRecordInserted_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(RankingAfterNewRecordInserted_Test ${INTEGRATION_TEST_LIBS} ${THREADING_TEST_LIBS})	
LIST(APPEND INTEGRATION_TESTS RankingAfterNewRecordInserted_Test)

ADD_EXECUTABLE(Pagination_Test Pagination_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(Pagination_Test ${INTEGRATION_TEST_LIBS} ${THREADING_TEST_LIBS})	
LIST(APPEND INTEGRATION_TESTS Pagination_Test)

ADD_EXECUTABLE(Lookup_Test Lookup_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(Lookup_Test ${INTEGRATION_TEST_LIBS} ${THREADING_TEST_LIBS})	
LIST(APPEND INTEGRATION_TESTS Lookup_Test)

ADD_EXECUTABLE(GeoCompleteTermSearch_Test GeoCompleteTermSearch_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(GeoCompleteTermSearch_Test ${INTEGRATION_TEST_LIBS} ${THREADING_TEST_LIBS})	
LIST(APPEND INTEGRATION_TESTS GeoCompleteTermSearch_Test)

ADD_CUSTOM_TARGET(build_integration_test ALL DEPENDS ${INTEGRATION_TESTS})
foreach (target ${INTEGRATION_TESTS})
    ADD_DEPENDENCIES(${target} srch2_core)
endforeach(target)
