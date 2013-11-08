
/*****************************************************************************
 *                                                                           *
 *              AUTHOR : RJ ATWAL                                            *
 *                                                                           *
 *                                                                           * 
  ****************************************************************************/

#include "StringAttribute.h"

namespace JNIClass {
  struct RefiningString : StringAttribute {
    RefiningString(JNIEnv*& env, jclass classPtr, jmethodID getValue,
        jmethodID constructor)
      : StringAttribute(env, classPtr, getValue, constructor) {}
  };
}
