#include "com_srch2_StringEngine.h"
#include "StringEngine.h"

/** Extracts the pointer to the c++ part of the StringEngine, handed down from
    the java part of the StringEngine by the given handle. The c++ part of the
    StringEngine is also informed of its JVM's current state. 
*/  
inline
StringEngine* dereferenceStringEngineHandle(JNIEnv *env, jlong handle) {
  StringEngine *rtn= (StringEngine*) handle;
  rtn->env= env;
  return rtn;
}

/* Initializes a new c++ StringEngine part with the instance language of its
   Java counterpart. The heap memory location of this new part is returned to
   its Java counterpart, so that it can be used as a handle to directed future
   bridged function calls.
*/
jlong Java_com_srch2_StringEngine_createStringEngine(JNIEnv *env,
    jobject, jclass searchableStringClassPtr,
    jobject searchableStringMethodgetValue,
    jobject searchableStringConstructor) {
  
  return 
    (jlong) new StringEngine(
      /* Creates a new permanent handle to the SearchableString Class. The new
         global reference creates a new handle object on the heap which
         references the same location as java handle referenced in the
         ClassPtr parameter; this call must be done in case the ClassPtr
         references a handle on the Java Stack, and thus will not persist for
         the next bridged function call. The persistent of this handle will
         also prevent Java from garbage collecting the SearchableString Class,
         even when no instances are present, since, a strong reference to it 
         will always be present.
       */
      (jclass) env->NewGlobalRef(searchableStringClassPtr),
      /* Extracts the constant JNI location of the given Method parameter. The
         parameter is a handle to a handle object which points to a given
         method; the extracted JNI location directly maps the method, relative
         to its associated class. This dramatically reduces lookup time; and, 
         the location remains constant, as long as its associated class
         persists, which is ensured by the above call.
       */
      env->FromReflectedMethod(searchableStringMethodgetValue),
      env->FromReflectedMethod(searchableStringConstructor));
}

/** Stores the SearchableString value passed down from the Java side of
    StringEngine, in the c++ side reference by the given handle */
void Java_com_srch2_StringEngine_setString
  (JNIEnv *env, jobject, jlong handle, jobject string) {
  dereferenceStringEngineHandle(env, handle)->setString(string);
}
/** Stores the given SearchableString's internalValue in this SearchableString
  */
void StringEngine::setString(jobject string) {
  assert(JNI_TRUE == searchableString.isInstance(string));
  this->value = searchableString.toString(string);
}

/** Returns the SearchableString equivalant of the string value stored in the
    c++ part of the StringEngine referenced by the given handle to the Java
    side of the StringEngine.
*/ 
jobject Java_com_srch2_StringEngine_getString (JNIEnv *env,
    jobject, jlong handle) {
  return dereferenceStringEngineHandle(env, handle)->getString();
}
/** Return a SearchableString with value equivalant to the one stored by this
    StringEngine.
  */
jobject StringEngine::getString() {
  return searchableString.createNew(value);
}

/** Deletes the c++ part of the StringEngine pointed to by the given handle */
void Java_com_srch2_StringEngine_deleteStringEngine(JNIEnv *env, jobject,
    jlong handle) {
  delete dereferenceStringEngineHandle(env, handle);
}
