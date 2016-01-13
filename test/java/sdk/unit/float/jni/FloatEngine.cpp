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
#include "com_srch2_FloatEngine.h"
#include "FloatEngine.h"

/** Extracts the pointer to the c++ part of the FloatEngine, handed down from
    the java part of the FloatEngine by the given handle. The c++ part of the
    FloatEngine is also informed of its JVM's current state. 
*/  
inline
FloatEngine* dereferenceFloatEngineHandle(JNIEnv *env, jlong handle) {
  FloatEngine *rtn= (FloatEngine*) handle;
  rtn->env= env;
  return rtn;
}

/* Initializes a new c++ FloatEngine part with the instance language of its
   Java counterpart. The heap memory location of this new part is returned to
   its Java counterpart, so that it can be used as a handle to directed future
   bridged function calls.
*/
jlong Java_com_srch2_FloatEngine_createFloatEngine(JNIEnv *env,
    jclass, jobject refiningFloatMethodgetValue,
    jclass refiningFloatClassPtr, jobject refiningFloatConstructor) {

 
  return 
    (jlong) new FloatEngine(
       /* Extracts the constant JNI location of the given Method parameter. The
         parameter is a handle to a handle object which points to a given
         method; the extracted JNI location directly maps the method, relative
         to its associated class. This dramatically reduces lookup time; and, 
         the location remains constant, as long as its associated class
         persists, which is ensured by the above call. */
      env->FromReflectedMethod(refiningFloatMethodgetValue),
      /* Creates a new permanent handle to the RefiningFloat Class. The new
         global reference creates a new handle object on the heap which
         references the same location as java handle referenced in the
         ClassPtr parameter; this call must be done in case the ClassPtr
         references a handle on the Java Stack, and thus will not persist for
         the next bridged function call. The persistent of this handle will
         also prevent Java from garbage collecting the RefiningFloat Class,
         even when no instances are present, since, a strong reference to it 
         will always be present.
       */
      (jclass) env->NewGlobalRef(refiningFloatClassPtr), 
      env->FromReflectedMethod(refiningFloatConstructor));
}

/** Stores the Float Attribute's value passed down from the Java side of
    FloatEngine, in the c++ side reference by the given handle */
void Java_com_srch2_FloatEngine_setFloat
  (JNIEnv *env, jclass, jlong handle, jobject floatValue) {
  dereferenceFloatEngineHandle(env, handle)->setFloat(floatValue);
}
/** Stores the given Float Attributes's internalValue in this SearchableFloat
  */
void FloatEngine::setFloat(jobject floatValue) {
  /* it does not matter that refiningFloat is used for all Float Attributes
     since they all share a common getValue method call */
  this->value = refiningFloat.toValue(floatValue);
}

/** Returns the RefiningFloat equivalent of the float value stored in the
    c++ part of the FloatEngine referenced by the given handle to the Java
    side of the FloatEngine.
*/ 
jobject Java_com_srch2_FloatEngine_getRefiningFloat (JNIEnv *env,
    jclass, jlong handle) {
  return dereferenceFloatEngineHandle(env, handle)->getRefiningFloat();
}
/** Return a RefiningFloat with value equivalent to the one stored by this
    FloatEngine.
  */
jobject FloatEngine::getRefiningFloat() {
  return refiningFloat.createNew(value);
}

/** Deletes the c++ part of the FloatEngine pointed to by the given handle */
void Java_com_srch2_FloatEngine_deleteFloatEngine(JNIEnv *env, jclass,
    jlong handle) {
  delete dereferenceFloatEngineHandle(env, handle);
}

