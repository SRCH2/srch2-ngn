// $Id: doxygen_EXAMPLE.h 580 2010-10-19 01:49:32Z vijay $

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

/**
 * @file doxytest.h
 * Write description of source file here for dOxygen.
 *
 * @brief Can use "brief" tag to explicitly generate comments for file documentation.
 *
 */

///  Single line comment for dOxygen.

/**
   Write description of function here.
   The function should follow these comments.
   Use of "brief" tag is optional. (no point to it)

   The function arguments listed with "param" will be compared
   to the declaration and verified.

   @param[in]     inArg1 Description of first function argument.
   @param[out]    outArg2 Description of second function argument.
   @param[in,out] inoutArg3 Description of third function argument.
   @return Description of returned value.
 */


#ifndef __DOXYTEST_H_
#define __DOXYTEST_H_

class doxytest {
public:
    doxytest();
    virtual ~doxytest();
    ///comments on function1 while declaring it
    int function1(int inArg1, int& outArg2, int& inoutArg3);
};

/** Comments on C++ doxytest::function1 while defining it
 */
int doxytest::function1(int _inArg1,/**< Comment on variable goes here. */
        int& _outArg2, /**< Comment on variable goes here. */
        int& _inoutArg3)
{

    /// Single line comment for dOxygen.
    return 0;
}

#endif /* __DOXYTEST_H_ */
