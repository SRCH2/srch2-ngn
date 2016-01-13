
// Contents of platform.h

#ifndef _SRCH2Lib_DLLDEFINES_H_
#define _SRCH2Lib_DLLDEFINES_H_

/*
Buildfiles will define MyLibrary_EXPORTS on Windows where it is
configured to build a shared library. If you are going to use
another build system on windows or create the visual studio
projects by hand you need to define MyLibrary_EXPORTS when
building a DLL on windows.
By default, srch2 build infrastructure uses the Visual Studio
Compiler and builds Shared libraries on Windows platform.
*/
namespace srch2
{
namespace instantsearch
{


#if defined (_WIN32)
  #if defined(srch2_instantsearch_EXPORTS)
    #define  MYLIB_EXPORT __declspec(dllexport)
  #else
    #define  MYLIB_EXPORT __declspec(dllimport)
  #endif /* MyLibrary_EXPORTS */
#else /* defined (_WIN32) */
 #define MYLIB_EXPORT
#endif
}}

#endif /* _SRCH2Lib_DLLDEFINES_H_ */
