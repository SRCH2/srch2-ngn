INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/src/core/
    ${CMAKE_SOURCE_DIR}/src/sharding/
    ${CMAKE_SOURCE_DIR}/src/server/
    ${CMAKE_SOURCE_DIR}/src/wrapper/
    ${CMAKE_SOURCE_DIR}/src/
    ${CMAKE_BINARY_DIR}/include/
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/thirdparty/event/include
    ${CMAKE_SOURCE_DIR}/thirdparty/json/jsoncpp-src/include
        ${CMAKE_SOURCE_DIR}/thirdparty/gperftools/include
    ${CMAKE_SOURCE_DIR}/thirdparty/mongo-cxx-driver-v2.4/src/
)

set(GPERFTOOL_LIBS "")
IF(ENABLE_PROFILER)
    set(GPERFTOOL_LIBS ${CMAKE_SOURCE_DIR}/thirdparty/gperftools/lib/libprofiler.a ${CMAKE_SOURCE_DIR}/thirdparty/libunwind/lib/libunwind.a)
ENDIF()

SET(UNIT_TEST_LIBS ${Srch2InstantSearch_LIBRARIES} ${Boost_LIBRARIES} ${CMAKE_REQUIRED_LIBRARIES} ${GPERFTOOL_LIBS})
SET( jsoncpp_LIBRARY ${CMAKE_SOURCE_DIR}/thirdparty/json/jsoncpp-src/build/libjsoncpp.a)
set(MONGO_CLIENT_LIBRARY ${CMAKE_SOURCE_DIR}/thirdparty/mongo-cxx-driver-v2.4/libmongoclient.a)

#ADD_EXECUTABLE(ConfigManager_Test ConfigManager_Test.cpp $<TARGET_OBJECTS:WRAPPER_OBJECTS> $<TARGET_OBJECTS:SHARDING_OBJECTS> $<TARGET_OBJECTS:SERVER_OBJECTS>)
#TARGET_LINK_LIBRARIES(ConfigManager_Test
#                        ${Srch2InstantSearch_LIBRARIES} 
#                        ${jsoncpp_LIBRARY}  ${CMAKE_SOURCE_DIR}/thirdparty/event/lib/libevent.a 
#                        ${MONGO_CLIENT_LIBRARY} ${Boost_LIBRARIES} ${CMAKE_REQUIRED_LIBRARIES}  ${GPERFTOOL_LIBS}
#                    )
#LIST(APPEND SHARD_UNIT_TESTS ConfigManager_Test)
ADD_EXECUTABLE(Serialization_Test Serialization_Test.cpp $<TARGET_OBJECTS:WRAPPER_OBJECTS> $<TARGET_OBJECTS:SHARDING_OBJECTS> $<TARGET_OBJECTS:SERVER_OBJECTS>)
TARGET_LINK_LIBRARIES(Serialization_Test
                        ${Srch2InstantSearch_LIBRARIES} 
                        ${jsoncpp_LIBRARY}  ${CMAKE_SOURCE_DIR}/thirdparty/event/lib/libevent.a 
                        ${MONGO_CLIENT_LIBRARY} ${Boost_LIBRARIES} ${CMAKE_REQUIRED_LIBRARIES}  ${GPERFTOOL_LIBS}
                    )
LIST(APPEND SHARD_UNIT_TESTS Serialization_Test)


 ADD_EXECUTABLE(TransportInitializationTest transport/TransportManagerInitializationTest.cpp $<TARGET_OBJECTS:WRAPPER_OBJECTS> $<TARGET_OBJECTS:SHARDING_OBJECTS> $<TARGET_OBJECTS:SERVER_OBJECTS>)
TARGET_LINK_LIBRARIES(TransportInitializationTest 
                        ${Srch2InstantSearch_LIBRARIES} 
                        ${jsoncpp_LIBRARY}  ${CMAKE_SOURCE_DIR}/thirdparty/event/lib/libevent.a 
                        ${MONGO_CLIENT_LIBRARY} ${Boost_LIBRARIES} ${CMAKE_REQUIRED_LIBRARIES}  ${GPERFTOOL_LIBS}
                    )
LIST(APPEND SHARD_UNIT_TESTS TransportInitializationTest)

ADD_EXECUTABLE(PendingMessagesTest transport/PendingMessagesTest.cpp $<TARGET_OBJECTS:WRAPPER_OBJECTS> $<TARGET_OBJECTS:SHARDING_OBJECTS> $<TARGET_OBJECTS:SERVER_OBJECTS>)
TARGET_LINK_LIBRARIES(PendingMessagesTest
                        ${Srch2InstantSearch_LIBRARIES} 
                        ${jsoncpp_LIBRARY}  ${CMAKE_SOURCE_DIR}/thirdparty/event/lib/libevent.a 
                        ${MONGO_CLIENT_LIBRARY} ${Boost_LIBRARIES} ${CMAKE_REQUIRED_LIBRARIES}  ${GPERFTOOL_LIBS}
                    )
LIST(APPEND SHARD_UNIT_TESTS PendingMessagesTest)

 ADD_EXECUTABLE(TMModuleTest transport/ModuleTest.cpp $<TARGET_OBJECTS:WRAPPER_OBJECTS> $<TARGET_OBJECTS:SHARDING_OBJECTS> $<TARGET_OBJECTS:SERVER_OBJECTS>)
TARGET_LINK_LIBRARIES(TMModuleTest
                        ${Srch2InstantSearch_LIBRARIES} 
                        ${jsoncpp_LIBRARY}  ${CMAKE_SOURCE_DIR}/thirdparty/event/lib/libevent.a 
                        ${MONGO_CLIENT_LIBRARY} ${Boost_LIBRARIES} ${CMAKE_REQUIRED_LIBRARIES}  ${GPERFTOOL_LIBS}
                    )
LIST(APPEND SHARD_UNIT_TESTS TMModuleTest)

 ADD_EXECUTABLE(SingleNodeIntraCommunicationTest transport/SingleNodeIntraCommunication.cpp $<TARGET_OBJECTS:WRAPPER_OBJECTS> $<TARGET_OBJECTS:SHARDING_OBJECTS> $<TARGET_OBJECTS:SERVER_OBJECTS>)
TARGET_LINK_LIBRARIES(SingleNodeIntraCommunicationTest 
                        ${Srch2InstantSearch_LIBRARIES} 
                        ${jsoncpp_LIBRARY}  ${CMAKE_SOURCE_DIR}/thirdparty/event/lib/libevent.a 
                        ${MONGO_CLIENT_LIBRARY} ${Boost_LIBRARIES} ${CMAKE_REQUIRED_LIBRARIES}  ${GPERFTOOL_LIBS}
                    )
LIST(APPEND SHARD_UNIT_TESTS SingleNodeIntraCommunicationTest)


ADD_EXECUTABLE(ResourceLockManager_Test ./shard_manager/ResourceLockManager_Test.cpp $<TARGET_OBJECTS:WRAPPER_OBJECTS> $<TARGET_OBJECTS:SHARDING_OBJECTS> $<TARGET_OBJECTS:SERVER_OBJECTS>)
TARGET_LINK_LIBRARIES(ResourceLockManager_Test
                        ${Srch2InstantSearch_LIBRARIES} 
                        ${jsoncpp_LIBRARY}  ${CMAKE_SOURCE_DIR}/thirdparty/event/lib/libevent.a 
                        ${MONGO_CLIENT_LIBRARY} ${Boost_LIBRARIES} ${CMAKE_REQUIRED_LIBRARIES}  ${GPERFTOOL_LIBS}
                    )
LIST(APPEND SHARD_UNIT_TESTS ResourceLockManager_Test)

ADD_EXECUTABLE(Notification_Test ./shard_manager/Notification_Test.cpp $<TARGET_OBJECTS:WRAPPER_OBJECTS> $<TARGET_OBJECTS:SHARDING_OBJECTS> $<TARGET_OBJECTS:SERVER_OBJECTS>)
TARGET_LINK_LIBRARIES(Notification_Test
                        ${Srch2InstantSearch_LIBRARIES} 
                        ${jsoncpp_LIBRARY}  ${CMAKE_SOURCE_DIR}/thirdparty/event/lib/libevent.a 
                        ${MONGO_CLIENT_LIBRARY} ${Boost_LIBRARIES} ${CMAKE_REQUIRED_LIBRARIES}  ${GPERFTOOL_LIBS}
                    )
LIST(APPEND SHARD_UNIT_TESTS Notification_Test)

ADD_EXECUTABLE(Metadata_Test ./shard_manager/Metadata_Test.cpp $<TARGET_OBJECTS:WRAPPER_OBJECTS> $<TARGET_OBJECTS:SHARDING_OBJECTS> $<TARGET_OBJECTS:SERVER_OBJECTS>)
TARGET_LINK_LIBRARIES(Metadata_Test
                        ${Srch2InstantSearch_LIBRARIES} 
                        ${jsoncpp_LIBRARY}  ${CMAKE_SOURCE_DIR}/thirdparty/event/lib/libevent.a 
                        ${MONGO_CLIENT_LIBRARY} ${Boost_LIBRARIES} ${CMAKE_REQUIRED_LIBRARIES}  ${GPERFTOOL_LIBS}
                    )
LIST(APPEND SHARD_UNIT_TESTS Metadata_Test)

ADD_CUSTOM_TARGET(build_shard_unit_test ALL DEPENDS ${SHARD_UNIT_TESTS} )
ADD_DEPENDENCIES(build_shard_unit_test srch2_core)
foreach (target ${SHARD_UNIT_TESTS})
    ADD_DEPENDENCIES(${target} srch2_core)
endforeach(target)
