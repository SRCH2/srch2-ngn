INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/include/
    ${Boost_INCLUDE_DIRS}
)

SET(IndexIntegration_Source  ${CMAKE_SOURCE_DIR}/test/integration/IntegrationTestHelper.h)
SET(MapSearchTest_Source  ${CMAKE_SOURCE_DIR}/test/integration/MapSearchTestHelper.h)
SET(INTEGRATION_TEST_LIBS ${Srch2InstantSearch_LIBRARIES} ${Boost_LIBRARIES} ${CMAKE_REQUIRED_LIBRARIES})

ADD_EXECUTABLE(BuildIndexesForIntegrationTest BuildIndexesForIntegrationTest.cpp ${IndexIntegration_Source} )
TARGET_LINK_LIBRARIES(BuildIndexesForIntegrationTest ${INTEGRATION_TEST_LIBS})

# CHENLI: deprecated
#ADD_EXECUTABLE(IndexIntegration_Test IndexIntegration_Test.cpp ${IndexIntegration_Source})
#TARGET_LINK_LIBRARIES(IndexIntegration_Test ${INTEGRATION_TEST_LIBS})

ADD_EXECUTABLE(CacheIntegration_Test CacheIntegration_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(CacheIntegration_Test ${INTEGRATION_TEST_LIBS})

ADD_EXECUTABLE(QueryStress_Test QueryStress_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(QueryStress_Test ${INTEGRATION_TEST_LIBS})

ADD_EXECUTABLE(DoubleQueryStress_Test DoubleQueryStress_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(DoubleQueryStress_Test  ${INTEGRATION_TEST_LIBS})

ADD_EXECUTABLE(EditDistance_Test EditDistance_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(EditDistance_Test ${INTEGRATION_TEST_LIBS})
 
#ADD_EXECUTABLE(Searcher_API2_Test Searcher_API2_Test.cpp ${IndexIntegration_Source})
#TARGET_LINK_LIBRARIES(Searcher_API2_Test ${INTEGRATION_TEST_LIBS})

ADD_EXECUTABLE(IndexUpdater_Test IndexUpdater_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(IndexUpdater_Test ${INTEGRATION_TEST_LIBS})

ADD_EXECUTABLE(MultipleLanguage_Test MultipleLanguage_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(MultipleLanguage_Test ${INTEGRATION_TEST_LIBS})

# ADD_EXECUTABLE(StemmerIntegration_Test StemmerIntegration_Test.cpp ${IndexIntegration_Source})
# TARGET_LINK_LIBRARIES(StemmerIntegration_Test  ${INTEGRATION_TEST_LIBS})
	
# ADD_EXECUTABLE(NormalizerIntegration_WithoutSerialization_Test NormalizerIntegration_WithoutSerialization_Test.cpp ${IndexIntegration_Source})
# TARGET_LINK_LIBRARIES(NormalizerIntegration_WithoutSerialization_Test  ${INTEGRATION_TEST_LIBS})
	
# ADD_EXECUTABLE(NormalizerIntegration_Test NormalizerIntegration_Test.cpp ${IndexIntegration_Source})
# TARGET_LINK_LIBRARIES(NormalizerIntegration_Test  ${INTEGRATION_TEST_LIBS})

ADD_EXECUTABLE(ThreadSafety_QueryStress_Test ThreadSafety_QueryStress_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(ThreadSafety_QueryStress_Test ${INTEGRATION_TEST_LIBS} ${THREADING_TEST_LIBS})

ADD_EXECUTABLE(ThreadSafetyReadWrite_Test ThreadSafetyReadWrite_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(ThreadSafetyReadWrite_Test ${INTEGRATION_TEST_LIBS} ${THREADING_TEST_LIBS})	

ADD_EXECUTABLE(KeywordReEncoding_Test KeywordReEncoding_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(KeywordReEncoding_Test ${INTEGRATION_TEST_LIBS} ${THREADING_TEST_LIBS})	

ADD_EXECUTABLE(Scalability_Test Scalability_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(Scalability_Test ${INTEGRATION_TEST_LIBS} ${THREADING_TEST_LIBS})	

ADD_EXECUTABLE(Scoring_Test Scoring_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(Scoring_Test ${INTEGRATION_TEST_LIBS} ${THREADING_TEST_LIBS})	

ADD_EXECUTABLE(GetAllResultsQuery_Test GetAllResultsQuery_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(GetAllResultsQuery_Test ${INTEGRATION_TEST_LIBS} ${THREADING_TEST_LIBS})	

ADD_EXECUTABLE(Serialization_on_Running_Test Serialization_on_Running_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(Serialization_on_Running_Test ${INTEGRATION_TEST_LIBS} ${THREADING_TEST_LIBS})	

ADD_EXECUTABLE(LargeInsertionAfterCommit_Test LargeInsertionAfterCommit_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(LargeInsertionAfterCommit_Test ${INTEGRATION_TEST_LIBS} ${THREADING_TEST_LIBS})	

ADD_EXECUTABLE(RankingAfterNewRecordInserted_Test RankingAfterNewRecordInserted_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(RankingAfterNewRecordInserted_Test ${INTEGRATION_TEST_LIBS} ${THREADING_TEST_LIBS})	

ADD_EXECUTABLE(Pagination_Test Pagination_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(Pagination_Test ${INTEGRATION_TEST_LIBS} ${THREADING_TEST_LIBS})	

ADD_EXECUTABLE(Lookup_Test Lookup_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(Lookup_Test ${INTEGRATION_TEST_LIBS} ${THREADING_TEST_LIBS})	

ADD_EXECUTABLE(GeoCompleteTermSearch_Test GeoCompleteTermSearch_Test.cpp ${IndexIntegration_Source})
TARGET_LINK_LIBRARIES(GeoCompleteTermSearch_Test ${INTEGRATION_TEST_LIBS} ${THREADING_TEST_LIBS})	

#ADD_EXECUTABLE(MarioQueryStress_Test MarioQueryStress_Test.cpp ${MapSearchTest_Source})
#TARGET_LINK_LIBRARIES(MarioQueryStress_Test  ${INTEGRATION_TEST_LIBS})

#ADD_EXECUTABLE(MarioIntegration_Test MarioIntegration_Test.cpp ${MapSearchTest_Source})
#TARGET_LINK_LIBRARIES(MarioIntegration_Test ${INTEGRATION_TEST_LIBS} )

#ADD_EXECUTABLE(Mario_ThreadSafety_QueryStress_Test Mario_ThreadSafety_QueryStress_Test.cpp ${IndexIntegration_Source})
#TARGET_LINK_LIBRARIES(Mario_ThreadSafety_QueryStress_Test ${INTEGRATION_TEST_LIBS} ${THREADING_TEST_LIBS})

#ADD_EXECUTABLE(Mario_ThreadSafety_ReadWrite_Test Mario_ThreadSafety_ReadWrite_Test.cpp ${IndexIntegration_Source})
#TARGET_LINK_LIBRARIES(Mario_ThreadSafety_ReadWrite_Test ${INTEGRATION_TEST_LIBS} ${THREADING_TEST_LIBS})

SET(INTEGRATION_TESTS 
#			BuildIndexesForIntegrationTest IndexIntegration_Test  
#			CacheIntegration_Test QueryStress_Test 
#        	EditDistance_Test IndexUpdater_Test
#        	ThreadSafety_QueryStress_Test 
#        	ThreadSafetyReadWrite_Test 
#        	DoubleQueryStress_Test NormalizerIntegration_Test
#        	NormalizerIntegration_WithoutSerialization_Test StringEncoding_Test
#        	Searcher_API2_Test 
#        	StemmerIntegration_Test
#        	MarioIntegration_Test MarioQueryStress_Test
#        	Mario_ThreadSafety_QueryStress_Test Mario_ThreadSafety_ReadWrite_Test
		)

#IF (BUILD_STATIC)
#	SET_TARGET_PROPERTIES( ${INTEGRATION_TESTS} PROPERTIES LINK_FLAGS "-Wl,-Bdynamic")
#ENDIF()

ADD_CUSTOM_TARGET(build_integration_test ALL DEPENDS ${INTEGRATION_TESTS})
