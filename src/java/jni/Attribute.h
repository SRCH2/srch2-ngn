#ifndef __ATTRIBUTE_H__
#define __ATTRIBUTE_H__

/*****************************************************************************
 *                                                                           *
 *              AUTHOR : RJ ATWAL                                            *
 *                                                                           *
 *                                                                           * 
  ****************************************************************************/

#include<jni.h>

#include "AttributeString.h"

namespace JNIClass {
  /* A wrapper around the java Attribute class */
  template <typename T>
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

    /** creates a new Attribute value with the equivalent value as the given
        c++ instance with the function C::makeJValue applied to it */
    template <typename C> jobject createNew(T& content) const;
    /** creates a new Attribute java instance with the equivalant value
        of the given c++ object */
    jobject createNew(T& content) const;

    /** Converts the given Attribute java instance to an instance of the 
        given c++ type using the provided Function: C::convertJavaToCPP */
    template <typename C> T toValue(jobject&) const;
    /** Directly constructs an instance of the given c++ type from the given
        Attribute java instance. */
    T toValue(jobject&) const;

    ~Attribute() {
      env->DeleteGlobalRef(classPtr);
    }
  };

  template <typename T>
  jboolean Attribute<T>::isInstance(jobject obj) const {
    return env->IsAssignableFrom(env->GetObjectClass(obj), classPtr);
  }
  
  /*  ---- toValue Specializations --- */
  
  template <typename T> template <typename C> inline  
  T JNIClass::Attribute<T>::toValue(jobject& obj) const {
    return T(C::convertJavaToCPP(*env, env->CallObjectMethod(obj, getValue)));
  }
  
  template <typename T>
  T JNIClass::Attribute<T>::toValue(jobject& obj) const {
    return T(env->CallObjectMethod(obj, getValue));
  }
  
  template <> inline  
  int Attribute<int>::toValue(jobject& obj) const {
    return env->CallIntMethod(obj, getValue);
  }
  
  template <> inline 
  float Attribute<float>::toValue(jobject& obj) const { 
    return env->CallFloatMethod(obj, getValue);
  }
  
  
  /* ---- createNew Specializations --- */
  
  template <typename T> template<typename C> inline
  jobject JNIClass::Attribute<T>::createNew(T& content) const {
    return env->NewObject(classPtr, constructor, C::makeJValue(*env, content));
  }
  
  template <typename T> 
  jobject Attribute<T>::createNew(T& content) const {
    return env->NewObject(classPtr, constructor, content);
  }


}

#endif /* __ATTRIBUTE_H__ */
