#ifndef __SEARCHABLE_STRING_H__
#define __SEARCHABLE_STRING_H__

/*****************************************************************************
 *                                                                           *
 *              AUTHOR : RJ ATWAL                                            *
 *                                                                           *
 *                                                                           * 
  ****************************************************************************/


#include<jni.h>
#include<string>

namespace JNIClass {
  /* A wrapper around the java SearchableString class */
  struct SearchableString {
    /* References the encapsulation Engine's storage of JVM context for each
       bridged call */
    JNIEnv*& env;
    /* The JVM location of the object used by the JVM to manipulate and
       comprehend SearchableString instances */
    jclass classPtr;
    /* The location of the method used to extract the UTF16 value of a given
       SearchableString */
    jmethodID getValue;
    /* The location of the constructor used to create a SearchableString
       instance from a UTF16 encoded Java String */
    jmethodID constructor;

    SearchableString(JNIEnv*& env, jclass classPtr, jmethodID getValue,
        jmethodID constructor)
      : env(env), classPtr(classPtr), getValue(getValue),
        constructor(constructor) {}
    /** converts a SearchableString java instance, with the assumed encoding
        of UTF16, into a c++ string, encoded in UTF8 */
    std::string toString(jobject srch2String) const;
    /** test if jobject is an instance of the java SearchableString class */
    jboolean isInstance(jobject obj) const;
    /** creates a new SearchableString java instance, encoded in UTF16,
        with the equivalant value of the given c++ string encoded in UTF8 */
    jobject createNew(std::string&) const;

    ~SearchableString() {
      env->DeleteGlobalRef(classPtr);
    }
  };
}

#endif /* __SEARCHABLE_STRING_H__ */
