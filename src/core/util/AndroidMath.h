/*
 * =====================================================================================
 *
 *       Filename:  AndroidMath.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/21/2014 05:46:30 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *
 * =====================================================================================
 */

#ifndef __SRCH2_UTIL_ANDROID_MATH_H__
#define __SRCH2_UTIL_ANDROID_MATH_H__

#ifdef ANDROID
    double inline log2(double x) { return log(x) / log (2);  }
#endif

#endif
