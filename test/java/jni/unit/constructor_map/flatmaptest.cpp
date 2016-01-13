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
#include "jni.h"
#include "initJNI.h"

#include<cassert>
#include<cstddef>
#include<cstdio>

int main() {
  jobject array[14];
  jobject map;
  
  /* Creates a map backed by the given array */
  map.array= array;
  
  /* a flat array with depth filled in by self-references; for example,
     the first element of the array contains a pointer to the arrray
     itself */
  array[JavaSDKClasses::JAVA_LANG_CLASS].array = array;
  array[JavaSDKClasses::JAVA_REFLECT_FIELD].array = array;
  array[JavaSDKClasses::COM_SRCH2_REFINING].methodID = 124;
  array[JavaSDKClasses::COM_SRCH2_SEARCHABLE].methodID = 3;
  array[JavaSDKClasses::COM_SRCH2_ATTRIBUTE].methodID = 4;
  array[JavaSDKClasses::COM_SRCH2_PRIMARY_KEY].methodID = 5;
  array[JavaSDKClasses::COM_SRCH2_REFININGFLOAT].methodID = 6;
  array[JavaSDKClasses::COM_SRCH2_REFININGUNSIGNED].methodID = 7;
  array[JavaSDKClasses::COM_SRCH2_REFININGDATE].methodID = 9;
  array[JavaSDKClasses::COM_SRCH2_REFININGINTERVAL].methodID = 10;
  array[JavaSDKClasses::COM_SRCH2_REFININGSTRING].methodID = 11;
  array[JavaSDKClasses::COM_SRCH2_INDEXABLESTRING].methodID = 12;
  array[JavaSDKClasses::COM_SRCH2_ATTRIBUTE_EXECEPTION].methodID = 13;
  array[JavaSDKClasses::COM_SRCH2_PARAMETER_MAPPING].array= array;

  JNIEnv e= JNIEnv();
  
  /* Creates a wrapping this map */
  JavaSDKClasses sdk(e, map ,  0, 0); 
  
  /* Checks that the map looks up directly correctly */
  assert(sdk.refiningClassPtr == 124);
  /* Checks that the map looks up depth, nested maps, correctly */
  assert(sdk.javaClassRef.ptr == map.cls);
  /* Checks second level look up: look ups on nested map */ 
  assert(sdk.javaClassRef.getFields == 124);
  /* Checks last map element edge case */
  assert(sdk.parameterMappingClassRef.ptr == map.cls);

  return 0;
}
