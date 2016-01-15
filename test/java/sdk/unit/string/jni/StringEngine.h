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
#include "Attribute.h"
#include<string>
#include<cassert>

class StringEngine {
 
  /** Encapsulates the instance language around a Java RefiningString object
      used by the particular JVM housing the Java side of this Engine */
  const JNIClass::Attribute<std::string>
    refiningString, searchableString, indexedString;

  /*  The stored value of this Engine */
  std::string value;

 public:
  /* Warning: this variable make this class thread unsafe, only
     a single thread may access a given instance of this class.
  
     The JVM context of the Java side of this Engine on a particular bridged
     call.
   */
  JNIEnv *env;

  StringEngine(jmethodID getValue,
      jclass refiningStringClassPtr, jmethodID refiningConstructor,
      jclass searchableStringClassPtr, jmethodID searchableConstructor,
      jclass indexedStringClassPtr, jmethodID indexedConstructor)
    : refiningString(this->env, refiningStringClassPtr,
          getValue, searchableConstructor),
      searchableString(this->env, searchableStringClassPtr,
         getValue, searchableConstructor),
      indexedString(this->env, indexedStringClassPtr,
          getValue, indexedConstructor) {}

  /* returns a Java RefiningString instance with value equivalent
     to the string contained this Engine. */
  jobject getRefiningString();
  /* returns a Java SearchableString instance with value equivalent
     to the string contained this Engine. */
  jobject getSearchableString();
  /* returns a Java IndexedString instance with value equivalent
     to the string contained this Engine. */
  jobject getIndexedString();
 
  /* Stores a new string value in this Engine, equivalent to the value of
     given StringAttribute. */
  void setString(jobject);
};

