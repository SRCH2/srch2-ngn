
########
# In srch2::atlantis codebase, the api files (trunk/include/instantsearch/*.h) are  
# not directly used to create SWIG wrappers. SWIG doesnot handle macros well and api 
# files (trunk/include/instantsearch/*.h) contain "MYLIB_EXPORT" macro for creating 
# windows dll.
###########

FILE(GLOB SRCH2_API_RAW_FILES "*[^_p].h")

FOREACH(includeFile ${SRCH2_API_RAW_FILES})
    # The version number.
    set (MYLIB_EXPORT MYLIB_EXPORT)
    
    # configure a header file to pass some of the CMake settings
    # to the source code
    #message(STATUS "Configuring file ${includeFile}")
    #STRING(REGEX REPLACE <pattern> <replacement string> <target variable> <source string>)
    STRING(REGEX REPLACE "${CMAKE_SOURCE_DIR}" "" fileName ${includeFile})
    
    configure_file (${includeFile} "${CMAKE_BINARY_DIR}${fileName}") 
ENDFOREACH(includeFile)

FILE(GLOB SRCH2_API_FILES "${CMAKE_BINARY_DIR}/include/instantsearch/*.h")

#include_directories(. .. ../include)
# enable gcc specific stuff
#if (CMAKE_COMPILER_IS_GNUCC)
#    set_source_files_properties(${SRCH2_LIB_SRC} PROPERTIES COMPILE_FLAGS " -Werror -Wall -pedantic")
#else (MSVC)
#    set_source_files_properties(${SRCH2_LIB_SRC})
#endif ()

#INSTALL(FILES ${SRCH2_API_FILES} DESTINATION include/srch2/instantsearch/include/instantsearch)
