
/*****************************************************************************
 *                                                                           *
 *              AUTHOR : RJ ATWAL                                            *
 *                                                                           *
 *                                                                           * 
  ****************************************************************************/

#include "StringAttribute.h"

namespace JNIClass {
  struct SearchableString : StringAttribute {

    SearchableString(JNIEnv*& env, jclass classPtr, jmethodID getValue,
        jmethodID constructor)
      : StringAttribute(env, classPtr, getValue, constructor) {}
  };
}

