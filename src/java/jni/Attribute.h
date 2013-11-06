#ifndef __ATTRIBUTE_H__
#define __ATTRIBUTE_H__

/*****************************************************************************
 *                                                                           *
 *              AUTHOR : RJ ATWAL                                            *
 *                                                                           *
 *                                                                           * 
  ****************************************************************************/

#include<jni.h>
#include<string>

namespace JNIClass {
  /* A wrapper around the java Attribute class */
  struct Attribute {
    /* References the encapsulation Engine's storage of JVM context for each
       bridged call */
    JNIEnv*& env;
    /* The JVM location of the object used by the JVM to manipulate and
       comprehend this Attribute's instances */
    jclass classPtr;
    /* The location of the method used to extract the internal value of a given
       Attribute */
    jmethodID getValue;
    /* The location of the constructor used to create this Attribute's
       instance */
    jmethodID constructor;

    Attribute(JNIEnv*& env, jclass classPtr, jmethodID getValue,
        jmethodID constructor)
      : env(env), classPtr(classPtr), getValue(getValue),
        constructor(constructor) {}
 
    /** test if jobject is an instance of the java StringAttribute class */
    jboolean isInstance(jobject obj) const;
    /** creates a new Attribute java instance with the equivalant value
        of the given c++ object */
    jobject createNew(jvalue& content) const;

    ~Attribute() {
      env->DeleteGlobalRef(classPtr);
    }
  };
}

jboolean JNIClass::Attribute::isInstance(jobject obj) const {
  return env->IsAssignableFrom(env->GetObjectClass(obj), classPtr);
}

jobject JNIClass::Attribute::createNew(jvalue& content) const {
  return env->NewObject(classPtr, constructor, content);
}

#endif /* __ATTRIBUTE_H__ */
