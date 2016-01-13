
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
