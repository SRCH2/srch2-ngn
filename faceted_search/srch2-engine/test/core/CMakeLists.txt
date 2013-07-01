
# $Id$

#Reference:http://www.mail-archive.com/cmake@cmake.org/msg21493.html
##############

ENABLE_TESTING()

ADD_SUBDIRECTORY(unit)
ADD_SUBDIRECTORY(integration)

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

ADD_TEST(Trie_Test ${CMAKE_CURRENT_BINARY_DIR}/unit/Trie_Test "--verbose")

# CHENLI: deprecated
#ADD_TEST(ForwardIndex_Test  ${CMAKE_CURRENT_BINARY_DIR}/unit/ForwardIndex_Test "--verbose")

ADD_TEST(FieldBoost_Test ${CMAKE_CURRENT_BINARY_DIR}/unit/FieldBoost_Test "--verbose")

ADD_TEST(PositionIndex_Test  ${CMAKE_CURRENT_BINARY_DIR}/unit/PositionIndex_Test "--verbose")
	
ADD_TEST(InvertedIndex_Test  ${CMAKE_CURRENT_BINARY_DIR}/unit/InvertedIndex_Test "--verbose")

ADD_TEST(ActiveNode_Test  ${CMAKE_CURRENT_BINARY_DIR}/unit/ActiveNode_Test "--verbose")
ADD_TEST(Cache_Test  ${CMAKE_CURRENT_BINARY_DIR}/unit/Cache_Test "--verbose")
ADD_TEST(Compression_S16_Test  ${CMAKE_CURRENT_BINARY_DIR}/unit/Compression_S16_Test "--verbose")
	
ADD_TEST(TermVirtualList_Test  ${CMAKE_CURRENT_BINARY_DIR}/unit/TermVirtualList_Test "--verbose")

ADD_TEST(Licensing_Test  ${CMAKE_CURRENT_BINARY_DIR}/unit/Licensing_Test "--verbose")

# CHENLI: deprecated
#ADD_TEST(IndexIntegration_Test  ${CMAKE_CURRENT_BINARY_DIR}/integration/IndexIntegration_Test "--verbose")
	
ADD_TEST(IndexerInternal_Test  ${CMAKE_CURRENT_BINARY_DIR}/unit/IndexerInternal_Test "--verbose")
	
ADD_TEST(IndexSearcherInternal_Test  ${CMAKE_CURRENT_BINARY_DIR}/unit/IndexSearcherInternal_Test "--verbose")

ADD_TEST(Trie_Test ${CMAKE_CURRENT_BINARY_DIR}/unit/Trie_Test "--verbose")

ADD_TEST(Analyzer_Test ${CMAKE_CURRENT_BINARY_DIR}/unit/Analyzer_Test "--verbose")
    
ADD_TEST(ReadWriteMutex_Test ${CMAKE_CURRENT_BINARY_DIR}/unit/ReadWriteMutex_Test "--verbose")

ADD_TEST(ReadWriteMutexCondWait_Test ${CMAKE_CURRENT_BINARY_DIR}/unit/ReadWriteMutexCondWait_Test "--verbose")

ADD_TEST(AttributedBasedSearch_Test ${CMAKE_CURRENT_BINARY_DIR}/unit/AttributedBasedSearch_Test "--verbose")
SET_TESTS_PROPERTIES(AttributedBasedSearch_Test
  PROPERTIES ENVIRONMENT "index_dir=${UNIT_TEST_INDEX_DIR}attr_based_search/")
############INTEGRATION TESTS#################
    
SET(INTEGRATION_TEST_INDEX_DIR ${CMAKE_SOURCE_DIR}/test/core/integration/test_data/)
    
### BuildIndexesForIntegration_Test build the necessary indexes for Integration Tests ###
ADD_TEST(BuildIndexesForIntegration_Test  ${CMAKE_CURRENT_BINARY_DIR}/integration/BuildIndexesForIntegrationTest "--verbose")
SET_TESTS_PROPERTIES(BuildIndexesForIntegration_Test 
  PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}dblp/")

ADD_TEST(CacheIntegration_Test ${CMAKE_CURRENT_BINARY_DIR}/integration/CacheIntegration_Test "--verbose")
SET_TESTS_PROPERTIES(CacheIntegration_Test 
				PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}dblp/;query_file=${INTEGRATION_TEST_INDEX_DIR}stress-test-queries/QW-1.dat")
         
#SET_TESTS_PROPERTIES(QueryStress_Test PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}/dblp/;query_file=${INTEGRATION_TEST_INDEX_DIR}/stress-test-queries/factual-queries/categories.txt")

#CHENLI: According to Vijay, this case is no longer valid
#ADD_TEST(QueryStress_Test  ${CMAKE_CURRENT_BINARY_DIR}/integration/QueryStress_Test "--verbose")
#SET_TESTS_PROPERTIES(QueryStress_Test PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}dblp/;query_file=${INTEGRATION_TEST_INDEX_DIR}stress-test-queries/QW-1.dat")
   
#To create fuzzy queries. use "awk '{ print substr($0,2) }' QW-1.dat > QW-1.dat_fuzzy" 
ADD_TEST(DoubleQueryStress_Test  ${CMAKE_CURRENT_BINARY_DIR}/integration/DoubleQueryStress_Test "--verbose")
SET_TESTS_PROPERTIES(DoubleQueryStress_Test PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}dblp/;query_file=${INTEGRATION_TEST_INDEX_DIR}/stress-test-queries/QW-1.dat")
    
ADD_TEST(EditDistance_Test  ${CMAKE_CURRENT_BINARY_DIR}/integration/EditDistance_Test "--verbose")
SET_TESTS_PROPERTIES(EditDistance_Test 
  PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}small-test-data/;")

  
#ADD_TEST(Searcher_API2_Test  ${CMAKE_CURRENT_BINARY_DIR}/integration/Searcher_API2_Test "--verbose")
#SET_TESTS_PROPERTIES(Searcher_API2_Test 
#			PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}small-test-data/;")

ADD_TEST(IndexUpdater_Test  ${CMAKE_CURRENT_BINARY_DIR}/integration/IndexUpdater_Test "--verbose")
SET_TESTS_PROPERTIES(IndexUpdater_Test 
    				PROPERTIES ENVIRONMENT "data_file=${INTEGRATION_TEST_INDEX_DIR}dblp/dblp40000records.csv;index_dir=${INTEGRATION_TEST_INDEX_DIR}update-test-data/;")

ADD_TEST(MultipleLanguage_Test  ${CMAKE_CURRENT_BINARY_DIR}/integration/MultipleLanguage_Test "--verbose")
SET_TESTS_PROPERTIES(MultipleLanguage_Test 
				PROPERTIES ENVIRONMENT "dblp_index_dir=${INTEGRATION_TEST_INDEX_DIR}dblp/;small_index_dir=${INTEGRATION_TEST_INDEX_DIR}small-test-data/;")
	
ADD_TEST(ThreadSafety_QueryStress_Test  ${CMAKE_CURRENT_BINARY_DIR}/integration/ThreadSafety_QueryStress_Test "--verbose")
#SET_TESTS_PROPERTIES(ThreadSafety_QueryStress_Test 
#			PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}factual/;query_file=${INTEGRATION_TEST_INDEX_DIR}stress-test-queries/factual-queries/categories.txt")
SET_TESTS_PROPERTIES(ThreadSafety_QueryStress_Test PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}/dblp/;query_file=${INTEGRATION_TEST_INDEX_DIR}/stress-test-queries/factual-queries/categories.txt")

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/test/wrapper/conf/conf.ini-input ${CMAKE_CURRENT_BINARY_DIR}/wrapper/conf.ini-WrapperCreateIndexFromJsonFile_Test)
ADD_TEST(WrapperCreateIndexFromJsonFile_Test  ${CMAKE_CURRENT_BINARY_DIR}/../search-server/WrapperCreateIndexFromFile_Test "--config-file=${CMAKE_CURRENT_BINARY_DIR}/wrapper/conf.ini-WrapperCreateIndexFromJsonFile_Test")
		
ADD_TEST(ThreadSafetyReadWrite_Test  ${CMAKE_CURRENT_BINARY_DIR}/integration/ThreadSafetyReadWrite_Test "--verbose")
SET_TESTS_PROPERTIES(ThreadSafetyReadWrite_Test 
				PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}small-test-data/;")

ADD_TEST(KeywordReEncoding_Test  ${CMAKE_CURRENT_BINARY_DIR}/integration/KeywordReEncoding_Test "--verbose")
SET_TESTS_PROPERTIES(KeywordReEncoding_Test 
				PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}small-test-data/;")

ADD_TEST(LargeInsertionAfterCommit_Test ${CMAKE_CURRENT_BINARY_DIR}/integration/LargeInsertionAfterCommit_Test "--verbose")
SET_TESTS_PROPERTIES(LargeInsertionAfterCommit_Test PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}large-insertion/")
	  
ADD_TEST(RankingAfterNewRecordInserted_Test ${CMAKE_CURRENT_BINARY_DIR}/integration/RankingAfterNewRecordInserted_Test "--verbose")
SET_TESTS_PROPERTIES(RankingAfterNewRecordInserted_Test PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}ranking-newly-inserted/")
 
ADD_TEST(Pagination_Test ${CMAKE_CURRENT_BINARY_DIR}/integration/Pagination_Test "--verbose")
SET_TESTS_PROPERTIES(Pagination_Test PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}pagination-test/")
 
ADD_TEST(Scoring_Test ${CMAKE_CURRENT_BINARY_DIR}/integration/Scoring_Test "--verbose")
SET_TESTS_PROPERTIES(Scoring_Test PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}scoring-test/")
 
ADD_TEST(Serialization_on_Running_Test ${CMAKE_CURRENT_BINARY_DIR}/integration/Serialization_on_Running_Test "--verbose")
SET_TESTS_PROPERTIES(Serialization_on_Running_Test PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}serialize-on-run-test/")
 
ADD_TEST(GetAllResultsQuery_Test ${CMAKE_CURRENT_BINARY_DIR}/integration/GetAllResultsQuery_Test "--verbose")
SET_TESTS_PROPERTIES(GetAllResultsQuery_Test PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}all-results-query-test/")
 
ADD_TEST(Lookup_Test ${CMAKE_CURRENT_BINARY_DIR}/integration/Lookup_Test "--verbose")
SET_TESTS_PROPERTIES(Lookup_Test PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}ranking-newly-inserted/")
 
ADD_TEST(GeoCompleteTermSearch_Test ${CMAKE_CURRENT_BINARY_DIR}/integration/GeoCompleteTermSearch_Test "--verbose")
SET_TESTS_PROPERTIES(GeoCompleteTermSearch_Test PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}geo-complete/")



 
# CHENLI
# ADD_TEST(StemmerIntegration_Test  ${CMAKE_CURRENT_BINARY_DIR}/integration/StemmerIntegration_Test "--verbose")
# SET_TESTS_PROPERTIES(StemmerIntegration_Test PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}stemmer/;")

# CHENLI
# ADD_TEST(NormalizerIntegration_Test  ${CMAKE_CURRENT_BINARY_DIR}/integration/NormalizerIntegration_Test "--verbose")
# SET_TESTS_PROPERTIES(NormalizerIntegration_Test PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}stemmer/;")
    
ADD_TEST(QuadTree_Test  ${CMAKE_CURRENT_BINARY_DIR}/unit/QuadTree_Test "--verbose")
SET_TESTS_PROPERTIES(QuadTree_Test PROPERTIES ENVIRONMENT "directoryName=${CMAKE_SOURCE_DIR}/test/unit/test_data")
	  
ADD_TEST(GeoIndexUpdate_Test ${CMAKE_CURRENT_BINARY_DIR}/unit/GeoIndexUpdate_Test "--verbose")
SET_TESTS_PROPERTIES(GeoIndexUpdate_Test PROPERTIES ENVIRONMENT "directoryName=${CMAKE_SOURCE_DIR}/test/unit/test_data")
	
ADD_TEST(VariableLengthAttributeContainer_Test ${CMAKE_CURRENT_BINARY_DIR}/unit/VariableLengthAttributeContainer_Test "--verbose")  


ADD_TEST(NonSearchableAttribute_Test ${CMAKE_CURRENT_BINARY_DIR}/unit/NonSearchableAttribute_Test "--verbose")  




#ADD_TEST(MarioIntegration_Test  ${CMAKE_CURRENT_BINARY_DIR}/integration/MarioIntegration_Test "--verbose")
#SET_TESTS_PROPERTIES(MarioIntegration_Test PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}/;small_index_dir=${INTEGRATION_TEST_INDEX_DIR}/small-test-data/")
    
#ADD_TEST(MarioOneQueryStress_Test  ${CMAKE_CURRENT_BINARY_DIR}/integration/MarioOneQueryStress_Test "--verbose")
#SET_TESTS_PROPERTIES(MarioOneQueryStress_Test PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}stemmer/")
  
#ADD_TEST(MarioOneSearchCorrectness_Test  ${CMAKE_CURRENT_BINARY_DIR}/integration/MarioOneSearchCorrectness_Test "--verbose")
#SET_TESTS_PROPERTIES(MarioOneSearchCorrectness_Test PROPERTIES ENVIRONMENT "index_dir=${INTEGRATION_TEST_INDEX_DIR}stemmer/")


##############CREATE FOLDERS TO RUN TEST CASES######################### 

SET(directory "test/test")
EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E make_directory ${directory})

SET(directory "test/test1")
EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E make_directory ${directory})
