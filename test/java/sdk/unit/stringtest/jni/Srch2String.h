#include<jni.h>
#include<string>

namespace JNIClass {
  /* A wrapper around the java Srch2String class */
  struct Srch2String {
    JNIEnv*& env;
    jclass classPtr;
    jmethodID getValue;
    jmethodID constructor;

    Srch2String(JNIEnv*& env, jclass classPtr, jmethodID getValue,
        jmethodID constructor)
      : env(env), classPtr(classPtr), getValue(getValue),
        constructor(constructor) {}
    /** coverts a Srch2String java instance into a c++ string*/
    std::string toString(jobject srch2String) const;
    /** test if jobject is an instance of the java Srch2String class */
    jboolean isInstance(jobject obj) const;
    /** creates a new Srch2String java instance with a given value */
    jobject createNew(std::string&) const;

  };
}

