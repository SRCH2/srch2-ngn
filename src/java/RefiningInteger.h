
/*****************************************************************************
 *                                                                           *
 *              AUTHOR : RJ ATWAL                                            *
 *                                                                           *
 *                                                                           * 
  ****************************************************************************/

#include "Attribute.h"

namespace JNIClass {
  struct RefiningInteger : IntegerAttribute {
    RefiningInteger(JNIEnv*& env, jclass classPtr, jmethodID getValue,
        jmethodID constructor)
      : IntegerAttribute(env, classPtr, getValue, constructor) {}
  };
}
