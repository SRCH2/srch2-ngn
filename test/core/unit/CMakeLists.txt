INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/src/core/
    ${CMAKE_BINARY_DIR}/include/
    ${Boost_INCLUDE_DIRS}
)

set(GPERFTOOL_LIBS "")
IF(ENABLE_PROFILER)
    set(GPERFTOOL_LIBS "${CMAKE_SOURCE_DIR}/thirdparty/gperftools/lib/libprofiler.a -lunwind")
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

ADD_EXECUTABLE(Analyzer_Test Analyzer_Test.cpp)
TARGET_LINK_LIBRARIES(Analyzer_Test ${UNIT_TEST_LIBS})
LIST(APPEND UNIT_TESTS Analyzer_Test)

ADD_EXECUTABLE(Cache_Test Cache_Test.cpp)
TARGET_LINK_LIBRARIES(Cache_Test ${UNIT_TEST_LIBS})
LIST(APPEND UNIT_TESTS Cache_Test)

ADD_EXECUTABLE(Compression_S16_Test Compression_S16_Test.cpp)
TARGET_LINK_LIBRARIES(Compression_S16_Test ${UNIT_TEST_LIBS})
LIST(APPEND UNIT_TESTS Compression_S16_Test)

ADD_EXECUTABLE(TermVirtualList_Test TermVirtualList_Test.cpp)
TARGET_LINK_LIBRARIES(TermVirtualList_Test ${UNIT_TEST_LIBS})
LIST(APPEND UNIT_TESTS TermVirtualList_Test)

ADD_EXECUTABLE(IndexSearcherInternal_Test IndexSearcherInternal_Test.cpp)
TARGET_LINK_LIBRARIES(IndexSearcherInternal_Test ${UNIT_TEST_LIBS})
LIST(APPEND UNIT_TESTS IndexSearcherInternal_Test)

ADD_EXECUTABLE(Licensing_Test Licensing_Test.cpp)
TARGET_LINK_LIBRARIES(Licensing_Test ${UNIT_TEST_LIBS})
LIST(APPEND UNIT_TESTS Licensing_Test)
	
ADD_EXECUTABLE(Stemmer_Test Stemmer_Test.cpp)
TARGET_LINK_LIBRARIES(Stemmer_Test ${UNIT_TEST_LIBS} )
LIST(APPEND UNIT_TESTS Stemmer_Test)
	
ADD_EXECUTABLE(Normalizer_Test Normalizer_Test.cpp)
TARGET_LINK_LIBRARIES(Normalizer_Test ${UNIT_TEST_LIBS} )
LIST(APPEND UNIT_TESTS Normalizer_Test)

ADD_EXECUTABLE(ReadWriteMutex_Test ReadWriteMutex_Test.cpp)
TARGET_LINK_LIBRARIES(ReadWriteMutex_Test ${UNIT_TEST_LIBS} )
LIST(APPEND UNIT_TESTS ReadWriteMutex_Test)

ADD_EXECUTABLE(ReadWriteMutexCondWait_Test ReadWriteMutexCondWait_Test.cpp)
TARGET_LINK_LIBRARIES(ReadWriteMutexCondWait_Test ${UNIT_TEST_LIBS} )
LIST(APPEND UNIT_TESTS ReadWriteMutexCondWait_Test)

ADD_EXECUTABLE(AttributedBasedSearch_Test AttributedBasedSearch_Test.cpp)
TARGET_LINK_LIBRARIES(AttributedBasedSearch_Test ${UNIT_TEST_LIBS} )
LIST(APPEND UNIT_TESTS AttributedBasedSearch_Test)

ADD_EXECUTABLE(ForwardIndex_Performance_Test ForwardIndex_Performance_Test.cpp)
TARGET_LINK_LIBRARIES(ForwardIndex_Performance_Test ${UNIT_TEST_LIBS} )
LIST(APPEND UNIT_TESTS ForwardIndex_Performance_Test)

ADD_EXECUTABLE(QuadTree_Test QuadTree_Test.cpp)
TARGET_LINK_LIBRARIES(QuadTree_Test ${UNIT_TEST_LIBS})	
LIST(APPEND UNIT_TESTS QuadTree_Test)

ADD_EXECUTABLE(GeoIndexUpdate_Test GeoIndexUpdate_Test.cpp)
TARGET_LINK_LIBRARIES(GeoIndexUpdate_Test ${UNIT_TEST_LIBS})	
LIST(APPEND UNIT_TESTS GeoIndexUpdate_Test)

ADD_CUSTOM_TARGET(build_unit_test ALL DEPENDS ${UNIT_TESTS} )
ADD_DEPENDENCIES(build_unit_test srch2_core)
foreach (target ${UNIT_TESTS})
    ADD_DEPENDENCIES(${target} srch2_core)
endforeach(target)
