FIND_PACKAGE(Java)

list(APPEND TEST_NAMES stringtest)


FOREACH(FILE_VAR LISTS TEST_NAME)
  ADD_CUSTOM_COMMAND(
      TARGET ant_build
      COMMAND ${Java_JAVAC_EXECUTABLE}
      ARGS "-d ${CMAKE_CURRENT_BINARY_DIR}/${FILE_VAR}"
      ARGS "sourcepath ${CMAKE_CURRENT_SOURCE_DIR}/${FILE_VAR}/sdk:${CMAKE_BINARY_DIR}/src/java/build_dir/*.jar"
      ARGS "com/srch2/*.java"
      )
  ADD_CUSTOM_COMMAND(
      TARGET ant_build
      COMMAND ${JAVA_JAR_EXECUTABLE}
      ARGS "cf ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}.jar"
      ARGS ${CMAKE_CURRENT_BINARY_DIR}/${FILE_VAR}/com/srch2/*"
      )
ENDFOREACH()
