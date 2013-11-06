#ifndef __SEARCHABLE_STRING_H__
#define __SEARCHABLE_STRING_H__

/*****************************************************************************
 *                                                                           *
 *              AUTHOR : RJ ATWAL                                            *
 *                                                                           *
 *                                                                           * 
  ****************************************************************************/

#include "Attribute.h"
#include<jni.h>
#include<string>

namespace JNIClass {
  struct SearchableString : StringAttribute {

    SearchableString(JNIEnv*& env, jclass classPtr, jmethodID getValue,
        jmethodID constructor)
      : StringAttribute(env, classPtr, getValue, constructor) {}
  };
}

#endif /* __SEARCHABLE_STRING_H__ */
