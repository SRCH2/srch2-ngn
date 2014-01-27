
# $Id$

#Reference:http://www.mail-archive.com/cmake@cmake.org/msg21493.html
##############

ENABLE_TESTING()

ADD_SUBDIRECTORY(core/unit)
ADD_SUBDIRECTORY(core/integration)
ADD_SUBDIRECTORY(wrapper/unit)
ADD_SUBDIRECTORY(wrapper/integration)

SET(MEMORYCHECK_COMMAND "/usr/bin/valgrind")
SET(MEMORYCHECK_COMMAND_OPTIONS "${MEMORYCHECK_COMMAND_OPTIONS} --trace-children=yes --leak-check=full --show-reachable=yes")

IF(CMAKE_COMPILER_IS_GNUCC)
    MESSAGE(STATUS "TESTING Environment: Linux")
ENDIF()

#
# In order to run tests we will need to set the approriate environment
# variable so that the test program can locate its dependent DLL's. First
# we want to know what directory our dependent DLL was installed into:
# Determine which environment variable controls the locating of
# DLL's and set that variable.
#
SET(SRCH2_LICENSE_DIR ${CMAKE_SOURCE_DIR}/test/core/Developer_License)
############UNIT TESTS#################

MESSAGE(STATUS  "BUILDING TEST: ${CMAKE_CURRENT_BINARY_DIR}")

SET(UNIT_TEST_INDEX_DIR ${CMAKE_SOURCE_DIR}/test/core/unit/test_data/)

ADD_TEST(Logger_Test ${CMAKE_CURRENT_BINARY_DIR}/core/unit/Logger_Test "--verbose")

ADD_TEST(Trie_Test ${CMAKE_CURRENT_BINARY_DIR}/core/unit/Trie_Test "--verbose")

ADD_TEST(FieldBoost_Test ${CMAKE_CURRENT_BINARY_DIR}/core/unit/FieldBoost_Test "--verbose")

ADD_TEST(PositionIndex_Test  ${CMAKE_CURRENT_BINARY_DIR}/core/unit/PositionIndex_Test "--verbose")
	
ADD_TEST(InvertedIndex_Test  ${CMAKE_CURRENT_BINARY_DIR}/core/unit/InvertedIndex_Test "--verbose")

ADD_TEST(ActiveNode_Test  ${CMAKE_CURRENT_BINARY_DIR}/core/unit/ActiveNode_Test "--verbose")

ADD_TEST(Cowvector_Test  ${CMAKE_CURRENT_BINARY_DIR}/core/unit/Cowvector_Test "--verbose")

ADD_TEST(Cache_Test  ${CMAKE_CURRENT_BINARY_DIR}/core/unit/Cache_Test "--verbose")
ADD_TEST(Compression_S16_Test  ${CMAKE_CURRENT_BINARY_DIR}/core/unit/Compression_S16_Test "--verbose")
	
ADD_TEST(TermVirtualList_Test  ${CMAKE_CURRENT_BINARY_DIR}/core/unit/TermVirtualList_Test "--verbose")

ADD_TEST(Licensing_Test  ${CMAKE_CURRENT_BINARY_DIR}/core/unit/Licensing_Test "--verbose")

ADD_TEST(IndexerInternal_Test  ${CMAKE_CURRENT_BINARY_DIR}/core/unit/IndexerInternal_Test "--verbose")
	
ADD_TEST(QueryEvaluator_Test  ${CMAKE_CURRENT_BINARY_DIR}/core/unit/QueryEvaluator_Test "--verbose")

ADD_TEST(Trie_Test ${CMAKE_CURRENT_BINARY_DIR}/core/unit/Trie_Test "--verbose")

ADD_TEST(Analyzer_Test ${CMAKE_CURRENT_BINARY_DIR}/core/unit/Analyzer_Test "--verbose")
SET_TESTS_PROPERTIES(Analyzer_Test PROPERTIES ENVIRONMENT "dataDir=${CMAKE_SOURCE_DIR}/test/core/unit/test_data/analyzer")
    
ADD_TEST(ReadWriteMutex_Test ${CMAKE_CURRENT_BINARY_DIR}/core/unit/ReadWriteMutex_Test "--verbose")

ADD_TEST(ReadWriteMutexCondWait_Test ${CMAKE_CURRENT_BINARY_DIR}/core/unit/ReadWriteMutexCondWait_Test "--verbose")

ADD_TEST(AttributedBasedSearch_Test ${CMAKE_CURRENT_BINARY_DIR}/core/unit/AttributedBasedSearch_Test "--verbose")
SET_TESTS_PROPERTIES(AttributedBasedSearch_Test
  PROPERTIES ENVIRONMENT "index_dir=${UNIT_TEST_INDEX_DIR}attr_based_search/")

ADD_TEST(RecordSerializer_Test ${CMAKE_CURRENT_BINARY_DIR}/core/unit/RecordSerializer_Test "--verbose")
############INTEGRATION TESTS#################
    
SET(INTEGRATION_TEST_INDEX_DIR ${CMAKE_SOURCE_DIR}/test/core/integration/test_data/)
    
### BuildIndexesForIntegration_Test build the necessary indexes for Integration Tests ###
ADD_TEST(BuildIndexesForIntegration_Test  ${CMAKE_CURRENT_BINARY_DIR}/core/integration/BuildIndexesForIntegrationTest "--verbose")
SET_TESTS_PROPERTIES(BuildIndexesForIntegration_Test 
  PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}dblp/")

#ADD_TEST(CacheIntegration_Test ${CMAKE_CURRENT_BINARY_DIR}/core/integration/CacheIntegration_Test "--verbose")
#SET_TESTS_PROPERTIES(CacheIntegration_Test 
#			PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}dblp/;query_file=${INTEGRATION_TEST_INDEX_DIR}stress-test-queries/QW-1.dat")
         
#SET_TESTS_PROPERTIES(QueryStress_Test PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}/dblp/;query_file=${INTEGRATION_TEST_INDEX_DIR}/stress-test-queries/factual-queries/categories.txt")

#To create fuzzy queries. use "awk '{ print substr($0,2) }' QW-1.dat > QW-1.dat_fuzzy" 
ADD_TEST(DoubleQueryStress_Test  ${CMAKE_CURRENT_BINARY_DIR}/core/integration/DoubleQueryStress_Test "--verbose")
SET_TESTS_PROPERTIES(DoubleQueryStress_Test PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}dblp/;query_file=${INTEGRATION_TEST_INDEX_DIR}/stress-test-queries/QW-1.dat")
    
ADD_TEST(EditDistance_Test  ${CMAKE_CURRENT_BINARY_DIR}/core/integration/EditDistance_Test "--verbose")
SET_TESTS_PROPERTIES(EditDistance_Test 
  PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}small-test-data/;")

  
ADD_TEST(IndexUpdater_Test  ${CMAKE_CURRENT_BINARY_DIR}/core/integration/IndexUpdater_Test "--verbose")
SET_TESTS_PROPERTIES(IndexUpdater_Test 
    				PROPERTIES ENVIRONMENT "data_file=${INTEGRATION_TEST_INDEX_DIR}dblp/dblp40000records.csv;index_dir=${INTEGRATION_TEST_INDEX_DIR}update-test-data/;")

ADD_TEST(MultipleLanguage_Test  ${CMAKE_CURRENT_BINARY_DIR}/core/integration/MultipleLanguage_Test "--verbose")
SET_TESTS_PROPERTIES(MultipleLanguage_Test 
				PROPERTIES ENVIRONMENT "small_index_dir=${INTEGRATION_TEST_INDEX_DIR}small-test-data/;")

ADD_TEST(MultipleLanguage_2_Test  ${CMAKE_CURRENT_BINARY_DIR}/core/integration/MultipleLanguage_2_Test "--verbose")
SET_TESTS_PROPERTIES(MultipleLanguage_2_Test 
				PROPERTIES ENVIRONMENT "dblp_index_dir=${INTEGRATION_TEST_INDEX_DIR}dblp/;small_index_dir=${INTEGRATION_TEST_INDEX_DIR}small-test-data/;")
	
ADD_TEST(ThreadSafety_QueryStress_Test  ${CMAKE_CURRENT_BINARY_DIR}/core/integration/ThreadSafety_QueryStress_Test "--verbose")
#SET_TESTS_PROPERTIES(ThreadSafety_QueryStress_Test 
#			PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}factual/;query_file=${INTEGRATION_TEST_INDEX_DIR}stress-test-queries/factual-queries/categories.txt")
SET_TESTS_PROPERTIES(ThreadSafety_QueryStress_Test PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}/dblp/;query_file=${INTEGRATION_TEST_INDEX_DIR}/stress-test-queries/factual-queries/categories.txt")

ADD_TEST(ThreadSafetyReadWrite_Test  ${CMAKE_CURRENT_BINARY_DIR}/core/integration/ThreadSafetyReadWrite_Test "--verbose")
SET_TESTS_PROPERTIES(ThreadSafetyReadWrite_Test 
				PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}small-test-data/;")

ADD_TEST(KeywordReEncoding_Test  ${CMAKE_CURRENT_BINARY_DIR}/core/integration/KeywordReEncoding_Test "--verbose")
SET_TESTS_PROPERTIES(KeywordReEncoding_Test 
				PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}small-test-data/;")

ADD_TEST(LargeInsertionAfterCommit_Test ${CMAKE_CURRENT_BINARY_DIR}/core/integration/LargeInsertionAfterCommit_Test "--verbose")
SET_TESTS_PROPERTIES(LargeInsertionAfterCommit_Test PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}large-insertion/")
	  
ADD_TEST(RankingAfterNewRecordInserted_Test ${CMAKE_CURRENT_BINARY_DIR}/core/integration/RankingAfterNewRecordInserted_Test "--verbose")
SET_TESTS_PROPERTIES(RankingAfterNewRecordInserted_Test PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}ranking-newly-inserted/")
 
ADD_TEST(Pagination_Test ${CMAKE_CURRENT_BINARY_DIR}/core/integration/Pagination_Test "--verbose")
SET_TESTS_PROPERTIES(Pagination_Test PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}pagination-test/")
 
ADD_TEST(Scoring_Test ${CMAKE_CURRENT_BINARY_DIR}/core/integration/Scoring_Test "--verbose")
SET_TESTS_PROPERTIES(Scoring_Test PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}scoring-test/")
 
ADD_TEST(Serialization_on_Running_Test ${CMAKE_CURRENT_BINARY_DIR}/core/integration/Serialization_on_Running_Test "--verbose")
SET_TESTS_PROPERTIES(Serialization_on_Running_Test PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}serialize-on-run-test/")
 
ADD_TEST(GetAllResultsQuery_Test ${CMAKE_CURRENT_BINARY_DIR}/core/integration/GetAllResultsQuery_Test "--verbose")
SET_TESTS_PROPERTIES(GetAllResultsQuery_Test PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}all-results-query-test/")
 
ADD_TEST(Lookup_Test ${CMAKE_CURRENT_BINARY_DIR}/core/integration/Lookup_Test "--verbose")
SET_TESTS_PROPERTIES(Lookup_Test PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}ranking-newly-inserted/")
 
ADD_TEST(GeoCompleteTermSearch_Test ${CMAKE_CURRENT_BINARY_DIR}/core/integration/GeoCompleteTermSearch_Test "--verbose")
SET_TESTS_PROPERTIES(GeoCompleteTermSearch_Test PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}geo-complete/")
 
ADD_TEST(QuadTree_Test  ${CMAKE_CURRENT_BINARY_DIR}/core/unit/QuadTree_Test "--verbose")
SET_TESTS_PROPERTIES(QuadTree_Test PROPERTIES ENVIRONMENT "directoryName=${CMAKE_SOURCE_DIR}/test/core/unit/test_data")
	  
ADD_TEST(GeoIndexUpdate_Test ${CMAKE_CURRENT_BINARY_DIR}/core/unit/GeoIndexUpdate_Test "--verbose")
SET_TESTS_PROPERTIES(GeoIndexUpdate_Test PROPERTIES ENVIRONMENT "directoryName=${CMAKE_SOURCE_DIR}/test/core/unit/test_data")


ADD_TEST(VariableLengthAttributeContainer_Test ${CMAKE_CURRENT_BINARY_DIR}/core/unit/VariableLengthAttributeContainer_Test "--verbose")
ADD_TEST(ULEB128_Test ${CMAKE_CURRENT_BINARY_DIR}/core/unit/ULEB128_Test "--verbose")

ADD_TEST(PhraseSearch_Test ${CMAKE_CURRENT_BINARY_DIR}/core/unit/PhraseSearch_Test "--verbose")
SET_TESTS_PROPERTIES(PhraseSearch_Test PROPERTIES ENVIRONMENT "positionIndexFile=${CMAKE_SOURCE_DIR}/test/core/unit/test_data/phraseSearch/positionIndex.txt")

ADD_TEST(MergeByShortestList_Test ${CMAKE_CURRENT_BINARY_DIR}/core/unit/MergeByShortestList_Test "--verbose")

ADD_TEST(MergeSortedById_Test ${CMAKE_CURRENT_BINARY_DIR}/core/unit/MergeSortedById_Test "--verbose")

ADD_TEST(MergeTopK_Test ${CMAKE_CURRENT_BINARY_DIR}/core/unit/MergeTopK_Test "--verbose")

ADD_TEST(SortById_Test ${CMAKE_CURRENT_BINARY_DIR}/core/unit/SortById_Test "--verbose")

ADD_TEST(SortByScore_Test ${CMAKE_CURRENT_BINARY_DIR}/core/unit/SortByScore_Test "--verbose")

ADD_TEST(UnionSortedById_Test ${CMAKE_CURRENT_BINARY_DIR}/core/unit/UnionSortedById_Test "--verbose")

ADD_TEST(RandomAccessVerificationAnd_Test ${CMAKE_CURRENT_BINARY_DIR}/core/unit/RandomAccessVerificationAnd_Test "--verbose")

ADD_TEST(RandomAccessVerificationOr_Test ${CMAKE_CURRENT_BINARY_DIR}/core/unit/RandomAccessVerificationOr_Test "--verbose")

ADD_TEST(RandomAccessVerificationNot_Test ${CMAKE_CURRENT_BINARY_DIR}/core/unit/RandomAccessVerificationNot_Test "--verbose")


#wrapper related tests should be added below this line

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/test/wrapper/conf/conf.xml-input ${CMAKE_CURRENT_BINARY_DIR}/wrapper/integration/conf.xml-WrapperCreateIndexFromJsonFile_Test)
ADD_TEST(WrapperCreateIndexFromJsonFile_Test  ${CMAKE_CURRENT_BINARY_DIR}/wrapper/integration/WrapperCreateIndexFromFile_Test "--config-file=${CMAKE_CURRENT_BINARY_DIR}/wrapper/integration/conf.xml-WrapperCreateIndexFromJsonFile_Test")
ADD_TEST(PostProcessingFilter_Test ${CMAKE_CURRENT_BINARY_DIR}/wrapper/integration/PostProcessingFilter_Test "--verbose")

ADD_TEST(XMLParser_Test ${CMAKE_CURRENT_BINARY_DIR}/wrapper/integration/XMLParser_Test   "--verbose")
SET_TESTS_PROPERTIES(XMLParser_Test PROPERTIES ENVIRONMENT "XmlFilePath=${CMAKE_SOURCE_DIR}/test/wrapper/data/test_data/XmlParserTest.xml")

if (SYSTEM_TEST_ENABLED)
    ADD_TEST( System_Tests ${CMAKE_SOURCE_DIR}/test/wrapper/system_tests/main.sh ${CMAKE_SOURCE_DIR}/test/wrapper/system_tests ${CMAKE_BINARY_DIR}/src/server)
endif()

