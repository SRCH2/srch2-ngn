/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __ATTRIBUTE_H__
#define __ATTRIBUTE_H__

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
        given c++ type using the provided function: C::convertJavaToCPP */
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
