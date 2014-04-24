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
	

ADD_EXECUTABLE(ConfigManager_Test ConfigManager_Test.cpp)
TARGET_LINK_LIBRARIES(ConfigManager_Test ${UNIT_TEST_LIBS})
LIST(APPEND SHARD_UNIT_TESTS ConfigManager_Test)

ADD_CUSTOM_TARGET(build_shard_unit_test ALL DEPENDS ${SHARD_UNIT_TESTS} )
ADD_DEPENDENCIES(build_shard_unit_test srch2_core)
foreach (target ${SHARD_UNIT_TESTS})
    ADD_DEPENDENCIES(${target} srch2_core)
endforeach(target)
