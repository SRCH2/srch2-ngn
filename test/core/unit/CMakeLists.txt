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

SET(UNIT_TEST_LIBS ${Srch2InstantSearch_LIBRARIES} ${Boost_LIBRARIES} ${CMAKE_REQUIRED_LIBRARIES} ${GPERFTOOL_LIBS})
	
ADD_EXECUTABLE(Logger_Test Logger_Test.cpp)
TARGET_LINK_LIBRARIES(Logger_Test ${UNIT_TEST_LIBS})
LIST(APPEND UNIT_TESTS Logger_Test)

ADD_EXECUTABLE(Trie_Test Trie_Test.cpp)
TARGET_LINK_LIBRARIES(Trie_Test ${UNIT_TEST_LIBS})
LIST(APPEND UNIT_TESTS Trie_Test)

# CHENLI: deprecated
#ADD_EXECUTABLE(ForwardIndex_Test ForwardIndex_Test.cpp)
#TARGET_LINK_LIBRARIES(ForwardIndex_Test ${UNIT_TEST_LIBS})
	
ADD_EXECUTABLE(FieldBoost_Test FieldBoost_Test.cpp)
TARGET_LINK_LIBRARIES(FieldBoost_Test ${UNIT_TEST_LIBS})
LIST(APPEND UNIT_TESTS FieldBoost_Test)
	
ADD_EXECUTABLE(InvertedIndex_Test InvertedIndex_Test.cpp)
TARGET_LINK_LIBRARIES(InvertedIndex_Test ${UNIT_TEST_LIBS})
LIST(APPEND UNIT_TESTS InvertedIndex_Test)
	
ADD_EXECUTABLE(PositionIndex_Test PositionIndex_Test.cpp)
TARGET_LINK_LIBRARIES(PositionIndex_Test ${UNIT_TEST_LIBS})
LIST(APPEND UNIT_TESTS PositionIndex_Test)

ADD_EXECUTABLE(IndexerInternal_Test IndexerInternal_Test.cpp)
TARGET_LINK_LIBRARIES(IndexerInternal_Test ${UNIT_TEST_LIBS})
LIST(APPEND UNIT_TESTS IndexerInternal_Test)

ADD_EXECUTABLE(ActiveNode_Test ActiveNode_Test.cpp)
TARGET_LINK_LIBRARIES(ActiveNode_Test ${UNIT_TEST_LIBS})
LIST(APPEND UNIT_TESTS ActiveNode_Test)

ADD_EXECUTABLE(Cowvector_Test Cowvector_Test.cpp)
TARGET_LINK_LIBRARIES(Cowvector_Test ${UNIT_TEST_LIBS})
LIST(APPEND UNIT_TESTS Cowvector_Test)

ADD_EXECUTABLE(Analyzer_Test Analyzer_Test.cpp)
TARGET_LINK_LIBRARIES(Analyzer_Test ${UNIT_TEST_LIBS})
LIST(APPEND UNIT_TESTS Analyzer_Test)

ADD_EXECUTABLE(Cache_Test Cache_Test.cpp)
TARGET_LINK_LIBRARIES(Cache_Test ${UNIT_TEST_LIBS})
LIST(APPEND UNIT_TESTS Cache_Test)

ADD_EXECUTABLE(CacheManager_Test CacheManager_Test.cpp)
TARGET_LINK_LIBRARIES(CacheManager_Test ${UNIT_TEST_LIBS})
LIST(APPEND UNIT_TESTS CacheManager_Test)

ADD_EXECUTABLE(Compression_S16_Test Compression_S16_Test.cpp)
TARGET_LINK_LIBRARIES(Compression_S16_Test ${UNIT_TEST_LIBS})
LIST(APPEND UNIT_TESTS Compression_S16_Test)

ADD_EXECUTABLE(TermVirtualList_Test TermVirtualList_Test.cpp)
TARGET_LINK_LIBRARIES(TermVirtualList_Test ${UNIT_TEST_LIBS})
LIST(APPEND UNIT_TESTS TermVirtualList_Test)

ADD_EXECUTABLE(QueryEvaluator_Test QueryEvaluator_Test.cpp)
TARGET_LINK_LIBRARIES(QueryEvaluator_Test ${UNIT_TEST_LIBS})
LIST(APPEND UNIT_TESTS QueryEvaluator_Test)

ADD_EXECUTABLE(Licensing_Test Licensing_Test.cpp)
TARGET_LINK_LIBRARIES(Licensing_Test ${UNIT_TEST_LIBS} ${crypto_LIBRARY} ${CMAKE_REQUIRED_LIBRARIES} )
LIST(APPEND UNIT_TESTS Licensing_Test)
	
ADD_EXECUTABLE(Normalizer_Test Normalizer_Test.cpp)
TARGET_LINK_LIBRARIES(Normalizer_Test ${UNIT_TEST_LIBS} )
LIST(APPEND UNIT_TESTS Normalizer_Test)

ADD_EXECUTABLE(AttributedBasedSearch_Test AttributedBasedSearch_Test.cpp)
TARGET_LINK_LIBRARIES(AttributedBasedSearch_Test ${UNIT_TEST_LIBS} )
LIST(APPEND UNIT_TESTS AttributedBasedSearch_Test)

ADD_EXECUTABLE(ForwardIndex_Performance_Test ForwardIndex_Performance_Test.cpp)
TARGET_LINK_LIBRARIES(ForwardIndex_Performance_Test ${UNIT_TEST_LIBS} )
LIST(APPEND UNIT_TESTS ForwardIndex_Performance_Test)

ADD_EXECUTABLE(ULEB128_Test ULEB128_Test.cpp)
TARGET_LINK_LIBRARIES(ULEB128_Test ${UNIT_TEST_LIBS})  
LIST(APPEND UNIT_TESTS ULEB128_Test)

ADD_EXECUTABLE(PhraseSearch_Test PhraseSearch_Test.cpp)
TARGET_LINK_LIBRARIES(PhraseSearch_Test ${UNIT_TEST_LIBS})  
LIST(APPEND UNIT_TESTS PhraseSearch_Test)

ADD_EXECUTABLE(Highlighter_Test Highlighter_Test.cpp)
TARGET_LINK_LIBRARIES(Highlighter_Test ${UNIT_TEST_LIBS})  
LIST(APPEND UNIT_TESTS Highlighter_Test)

ADD_EXECUTABLE(RecordSerializer_Test RecordSerializer_Test.cpp)
TARGET_LINK_LIBRARIES(RecordSerializer_Test  ${UNIT_TEST_LIBS})  
LIST(APPEND UNIT_TESTS RecordSerializer_Test)

ADD_EXECUTABLE(MergeByShortestList_Test physical_plan/MergeByShortestList_Test.cpp)
TARGET_LINK_LIBRARIES(MergeByShortestList_Test ${UNIT_TEST_LIBS})  
LIST(APPEND UNIT_TESTS MergeByShortestList_Test)

ADD_EXECUTABLE(MergeSortedById_Test physical_plan/MergeSortedById_Test.cpp)
TARGET_LINK_LIBRARIES(MergeSortedById_Test ${UNIT_TEST_LIBS})  
LIST(APPEND UNIT_TESTS MergeSortedById_Test)

ADD_EXECUTABLE(MergeTopK_Test physical_plan/MergeTopK_Test.cpp)
TARGET_LINK_LIBRARIES(MergeTopK_Test ${UNIT_TEST_LIBS})  
LIST(APPEND UNIT_TESTS MergeTopK_Test)

ADD_EXECUTABLE(SortById_Test physical_plan/SortById_Test.cpp)
TARGET_LINK_LIBRARIES(SortById_Test ${UNIT_TEST_LIBS})  
LIST(APPEND UNIT_TESTS SortById_Test)

ADD_EXECUTABLE(SortByScore_Test physical_plan/SortByScore_Test.cpp)
TARGET_LINK_LIBRARIES(SortByScore_Test ${UNIT_TEST_LIBS})  
LIST(APPEND UNIT_TESTS SortByScore_Test)

ADD_EXECUTABLE(UnionSortedById_Test physical_plan/UnionSortedById_Test.cpp)
TARGET_LINK_LIBRARIES(UnionSortedById_Test ${UNIT_TEST_LIBS})  
LIST(APPEND UNIT_TESTS UnionSortedById_Test)

ADD_EXECUTABLE(RandomAccessVerificationAnd_Test physical_plan/RandomAccessVerificationAnd_Test.cpp)
TARGET_LINK_LIBRARIES(RandomAccessVerificationAnd_Test ${UNIT_TEST_LIBS})  
LIST(APPEND UNIT_TESTS RandomAccessVerificationAnd_Test)

ADD_EXECUTABLE(RandomAccessVerificationOr_Test physical_plan/RandomAccessVerificationOr_Test.cpp)
TARGET_LINK_LIBRARIES(RandomAccessVerificationOr_Test ${UNIT_TEST_LIBS})  
LIST(APPEND UNIT_TESTS RandomAccessVerificationOr_Test)

ADD_EXECUTABLE(RandomAccessVerificationNot_Test physical_plan/RandomAccessVerificationNot_Test.cpp)
TARGET_LINK_LIBRARIES(RandomAccessVerificationNot_Test ${UNIT_TEST_LIBS})  
LIST(APPEND UNIT_TESTS RandomAccessVerificationNot_Test)

ADD_EXECUTABLE(RecordBaseAccessControl_Test RecordBaseAccessControl_Test.cpp)
TARGET_LINK_LIBRARIES(RecordBaseAccessControl_Test ${UNIT_TEST_LIBS})  
LIST(APPEND UNIT_TESTS RecordBaseAccessControl_Test)

ADD_CUSTOM_TARGET(build_unit_test ALL DEPENDS ${UNIT_TESTS} )
ADD_DEPENDENCIES(build_unit_test srch2_core)
foreach (target ${UNIT_TESTS})
    ADD_DEPENDENCIES(${target} srch2_core)
endforeach(target)
