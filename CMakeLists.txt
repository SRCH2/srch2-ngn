# CMake Build Environment
# This is the top-level CMakeLists file. The build is preformed "outside of source" in the build 
# directory. 
# USAGE: 
# 1) Create a new directory called build inside trunk directory.
# 2) Inside this directory, use the following command:
#
#        cmake -D[${option}={value}] -D[..] ${path_to_build_directory}
#
#OPTIONS USED:
#
#    1) DO_RELEASE: When set to ON, changes the build_type to release.
#    2) DO_PROFILE: When set to ON, adds profiler tags to linker.
#
#EXAMPLE:
#    terminal> cmake ..
#
# By default, cmake builds in debug mode with profiler tags turned off.
#
#    terminal> cmake -DDO_RELEASE=1 ..
# cmake build_type is release mode
#
#    terminal> cmake -DDO_DEBUG=0 -DDO_PROFILE=1 ..
########################################################################################################

# Project name is not mandatory, but you should use it
PROJECT(Srch2-Instant-Search)

# States that CMake required version must be >= 2.6
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.8)
MESSAGE(STATUS "CMake version: " ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})

IF(COMMAND cmake_policy)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND cmake_policy)

##SET this option to "on", if you want detailed makefile execution
SET(VERBOSE ON CACHE BOOL "Build verbose makefiles and output. Default ON")
SET(CMAKE_VERBOSE_MAKEFILE ${VERBOSE})
SET(BUILD_STATIC ON CACHE BOOL "Build Srch2 static library and binary. Default ON")
SET(BUILD_RELEASE ON CACHE BOOL "Build Release. Default ON")
SET(BUILD_TEST ON CACHE BOOL "Build tests. Default ON")
#SET(CMAKE_CXX_FLAGS_RELEASE "") 
IF (BUILD_STATIC)
   SET(CMAKE_EXE_LINKER_FLAGS -static)
   SET(CMAKE_FIND_LIBRARY_SUFFIXES .a)  # is this appliable for MAC too ?
ENDIF (BUILD_STATIC) 

#BUILD TYPE
IF(BUILD_RELEASE)
	MESSAGE(STATUS "BUILD_TYPE is RELEASE")
    SET (CMAKE_BUILD_TYPE Release)
    SET (DO_PROFILE OFF)
    SET (DO_COVERAGE OFF)
	SET (DO_ASSERT OFF)
ELSE(BUILD_RELEASE)
	MESSAGE(STATUS "BUILD_TYPE is DEBUG")
    SET (CMAKE_BUILD_TYPE Debug)
    SET (DO_PROFILE OFF)
    SET (DO_COVERAGE OFF)
	SET (DO_ASSERT ON)
ENDIF(BUILD_RELEASE)

# http://stackoverflow.com/questions/6578484/telling-gcc-directly-to-link-a-library-statically
SET(CMAKE_EXE_LINK_DYNAMIC_C_FLAGS)       # remove -Wl,-Bdynamic
SET(CMAKE_EXE_LINK_DYNAMIC_CXX_FLAGS)
SET(CMAKE_SHARED_LIBRARY_C_FLAGS)         # remove -fPIC
SET(CMAKE_SHARED_LIBRARY_CXX_FLAGS)
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)    # remove -rdynamic
SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}")

# Platform SpecIFic Compiler Flags
#IF (CMAKE_COMPILER_IS_GNUCXX)
    EXEC_PROGRAM( ${CMAKE_CXX_COMPILER}
                  ARGS  -dumpversion
                  OUTPUT_VARIABLE gcc_compiler_version)
    MESSAGE(STATUS "C++ compiler version: ${gcc_compiler_version}") 
    string(COMPARE LESS ${gcc_compiler_version} "4.6" _compiler_version_lt_4.6)
    
    if (_compiler_version_lt_4.6)
       SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wno-long-long -Wno-conversion -Wno-sign-compare -Wno-unused")
    else()
       SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wno-long-long -Wno-conversion -Wno-sign-compare -Wno-unused -Wno-int-to-pointer-cast")
    endif()
    SET(CMAKE_REQUIRED_LIBRARIES pthread stdc++ crypto ${CMAKE_REQUIRED_LIBRARIES})
    
	IF(DO_COVERAGE)
    	MESSAGE(STATUS "COVERAGE tags Turned ON")
        SET(COVERAGE_FLAGS "-fprofile-arcs -ftest-coverage")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COVERAGE_FLAGS}" )
        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${COVERAGE_FLAGS}" )
    ELSE()
        MESSAGE(STATUS "COVERAGE tags Turned OFF")    
    ENDIF()
 
	IF (DO_ASSERT)
		MESSAGE(STATUS "ASSERT Turned ON")
	ELSE()
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DASSERT_LEVEL=0")
		MESSAGE(STATUS "ASSERT Turned OFF")
	ENDIF()
    
    SET( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
    
    SET( Boost_ADDITIONAL_VERSIONS "1.34" "1.40.0" "1.40" "1.41")

    ## CHENLI
    IF (BUILD_STATIC)
      SET(Boost_USE_STATIC_LIBS ON)
    ENDIF ()
	
	IF (BUILD_RELEASE)
		SET(Boost_DEBUG OFF)
	ENDIF ()
	
	SET(sys_LIBRARY_DIRS	"/usr/lib64/" 
					"/lib/"
					"/lib64/"	
					)
					
	SET(Boost_USE_MULTITHREAD ON)
    FIND_PACKAGE( Boost COMPONENTS serialization program_options regex system thread REQUIRED )
    
	MESSAGE(STATUS "Boost_VERSION=" ${Boost_VERSION})
	MESSAGE(STATUS "Boost_LIBRARIES=" ${Boost_LIBRARIES})
	MESSAGE(STATUS "Boost_INCLUDE_DIRS=" ${Boost_INCLUDE_DIRS})
	
	# Finally the rt, dl and z library
	FIND_LIBRARY(rt_LIBRARY NAMES rt PATHS ${sys_LIBRARY_DIRS})
	FIND_LIBRARY(dl_LIBRARY NAMES dl PATHS ${sys_LIBRARY_DIRS})
	FIND_LIBRARY(z_LIBRARY NAMES z PATHS ${sys_LIBRARY_DIRS})
	
	MESSAGE(STATUS "librt = ${rt_LIBRARY}")
	MESSAGE(STATUS "libdl = ${dl_LIBRARY}")
	MESSAGE(STATUS "libz = ${z_LIBRARY}")	

	IF (BUILD64BIT)
		MESSAGE(STATUS "Buildling for 64 bit platform")
		LINK_DIRECTORIES( /usr/lib/x86_64-linux-gnu/
			  /usr/lib64/
			)
	ELSE ()
		MESSAGE(STATUS "Buildling for 32 bit platform")
		LINK_DIRECTORIES("/usr/lib/")
	ENDIF ()

	SET(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${OPENSSL_LIBRARIES} ${rt_LIBRARY} ${dl_LIBRARY} ${z_LIBRARY})

	#Add subdirectories to look for other CMakeList.txt
	ADD_SUBDIRECTORY(include/instantsearch)
	ADD_SUBDIRECTORY(src/core)

	# Building the wrapper layer in static build mode.
	# Setting up the Path to Srch2_InstantSearch_library
	IF (BUILD_STATIC)
		SET(Srch2InstantSearch_LIBRARIES ${CMAKE_BINARY_DIR}/src/core/libsrch2_core.a)
	ELSE (BUILD_STATIC)
		SET(Srch2InstantSearch_LIBRARIES ${CMAKE_BINARY_DIR}/src/core/libsrch2_core.so)
	ENDIF()

	ADD_SUBDIRECTORY(src/wrapper)
	ADD_SUBDIRECTORY(src/server)
	
	IF (BUILD_TEST)
		INCLUDE (CTest)
		ADD_SUBDIRECTORY(test)
		ADD_DEPENDENCIES(build_integration_test srch2_core)
		ADD_DEPENDENCIES(build_unit_test srch2_core) 
	ENDIF()
		
	#ADD_SUBDIRECTORY(examples)
	#ADD_DEPENDENCIES(examples_c++ srch2_lib)
	#ADD_DEPENDENCIES(examples_python srch2_lib)
	#ADD_SUBDIRECTORY(docs)
	#ADD_SUBDIRECTORY(swig)
	#ADD_DEPENDENCIES(build_swig srch2_lib)
	
#ENDIF()



####################################################################################################
#CPack tarballing code!

#INSTALL  
SET(INSTALL_AUXILLARY_FILES_DIR "" CACHE STRING "Directory Path of Auxillary files. e.g config file")
SET(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install")
INSTALL(DIRECTORY docs/ DESTINATION docs USE_SOURCE_PERMISSIONS PATTERN "*.mkd" EXCLUDE) 

INSTALL(FILES ${INSTALL_AUXILLARY_FILES_DIR}/readme.txt DESTINATION . PERMISSIONS OWNER_READ OWNER_WRITE)

INSTALL(FILES ${INSTALL_AUXILLARY_FILES_DIR}/srch2_config.ini DESTINATION conf PERMISSIONS OWNER_READ OWNER_WRITE)

INSTALL(DIRECTORY ${INSTALL_AUXILLARY_FILES_DIR}/srch2_data/ DESTINATION srch2_data USE_SOURCE_PERMISSIONS)

#Packaging
SET(CPACK_IGNORE_FILES "CMakeCache.txt;install_manifest.txt;/Testing/;/Makefile$;\\\\.cmake$;\\\\.tar.gz$;\\\\.so[.0-9]*$;/build/;/_CPack_Packages/;/CMakeFiles/;/CVS/;/\\\\.svn/;/\\\\.git/;\\\\.swp$;\\\\.#;/#")
#SET(CPACK_IGNORE_FILES "CMakeCache.txt;install_manifest.txt;/Testing/;/Makefile$;\\\\.tar.gz$;\\\\.so[.0-9]*$;/build/;/_CPack_Packages/;/CMakeFiles/;/CVS/;/\\\\.svn/;/\\\\.git/;\\\\.swp$;\\\\.#;/#")

SET(CPACK_SOURCE_IGNORE_FILES ${CPACK_IGNORE_FILES})
SET(CPACK_SOURCE_GENERATOR TGZ ZIP)
SET(CPACK_SOURCE_TOPLEVEL_TAG "")
SET(CPACK_PACKAGE_NAME "srch2-engine")
SET(CPACK_PACKAGE_DESCRIPTION "The Instant Typo-Tolerant Search Engine")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "www.srch2.com")
SET(CPACK_PACKAGE_CONTACT "contact@srch2.com")
SET(CPACK_PACKAGE_VENDOR "Srch2 Software Inc.")

# Note: Version should be passed as input to cmake file. e.g -DVERSION:STRING="3.1.0" 
SET(VERSION "" CACHE STRING "Version Number") 

SET(CPACK_GENERATOR "TGZ;")
SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${VERSION}")
INCLUDE(CPack)

# enable dashboard scripting
include(Dart)
SET (CTEST_PROJECT_NAME "Srch2 - Testing Framework")
