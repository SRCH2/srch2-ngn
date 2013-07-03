//$Id: platform.h 3014 2012-12-04 23:34:03Z oliverax $

/*
 * The Software is made available solely for use according to the License Agreement. Any reproduction
 * or redistribution of the Software not in accordance with the License Agreement is expressly prohibited
 * by law, and may result in severe civil and criminal penalties. Violators will be prosecuted to the
 * maximum extent possible.
 *
 * THE SOFTWARE IS WARRANTED, IF AT ALL, ONLY ACCORDING TO THE TERMS OF THE LICENSE AGREEMENT. EXCEPT
 * AS WARRANTED IN THE LICENSE AGREEMENT, SRCH2 INC. HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS WITH
 * REGARD TO THE SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES AND CONDITIONS OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT.  IN NO EVENT SHALL SRCH2 INC. BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF SOFTWARE.

 * Copyright © 2010 SRCH2 Inc. All rights reserved
 */

// Contents of platform.h

#ifndef _BimapleLib_DLLDEFINES_H_
#define _BimapleLib_DLLDEFINES_H_

/*
Buildfiles will define MyLibrary_EXPORTS on Windows where it is
configured to build a shared library. If you are going to use
another build system on windows or create the visual studio
projects by hand you need to define MyLibrary_EXPORTS when
building a DLL on windows.
By default, bimaple build infrastructure uses the Visual Studio
Compiler and builds Shared libraries on Windows platform.
*/
namespace bimaple
{
namespace instantsearch
{


#if defined (_WIN32)
  #if defined(bimaple_instantsearch_EXPORTS)
    #define  MYLIB_EXPORT __declspec(dllexport)
  #else
    #define  MYLIB_EXPORT __declspec(dllimport)
  #endif /* MyLibrary_EXPORTS */
#else /* defined (_WIN32) */
 #define MYLIB_EXPORT
#endif
}}

#endif /* _BimapleLib_DLLDEFINES_H_ */
