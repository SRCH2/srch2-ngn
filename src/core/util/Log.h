//$Id$

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

#ifndef LOG_H_
#define LOG_H_

namespace srch2
{
namespace instantsearch
{

/* We can print the stacktrace, so our assert is defined this way: */
#define bmAssertWithInfo(_c,_o,_e) ((_e)?(void)0 : (bmAssertWithInfo(_c,_o,#_e,__FILE__,__LINE__),_exit(1)))
#define bmAssert(_e) ((_e)?(void)0 : (bmAssert(#_e,__FILE__,__LINE__),_exit(1)))
#define bmPanic(_e) _redisPanic(#_e,__FILE__,__LINE__),_exit(1)

#ifndef LOG_LEVEL
#ifdef NDEBUG
// Release
#define LOG_LEVEL 1
#else
// Debug
#define LOG_LEVEL 0
#endif
#endif

#define LOG_REGION(level, logcode) do {\
	if (level >= LOG_LEVEL) {logcode;}\
} while(0)

}}
#endif /* LOG_H_ */
