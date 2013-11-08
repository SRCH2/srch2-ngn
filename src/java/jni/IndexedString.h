
/*****************************************************************************
 *                                                                           *
 *              AUTHOR : RJ ATWAL                                            *
 *                                                                           *
 *                                                                           * 
  ****************************************************************************/

#include "StringAttribute.h"

namespace JNIClass {
  struct IndexedString : StringAttribute {
    IndexedString(JNIEnv*& env, jclass classPtr, jmethodID getValue,
        jmethodID constructor)
      : StringAttribute(env, classPtr, getValue, constructor) {}
  };
}
