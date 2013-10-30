#include<jni.h>
#include<string>

namespace JNIClass {
  /* A wrapper around the java SearchableString class */
  struct SearchableString {
    JNIEnv*& env;
    jclass classPtr;
    jmethodID getValue;
    jmethodID constructor;

    SearchableString(JNIEnv*& env, jclass classPtr, jmethodID getValue,
        jmethodID constructor)
      : env(env), classPtr(classPtr), getValue(getValue),
        constructor(constructor) {}
    /** coverts a SearchableString java instance into a c++ string*/
    std::string toString(jobject srch2String) const;
    /** test if jobject is an instance of the java SearchableString class */
    jboolean isInstance(jobject obj) const;
    /** creates a new SearchableString java instance with a given value */
    jobject createNew(std::string&) const;

  };
}

