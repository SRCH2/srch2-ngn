#include "com_srch2_Srch2EngineTest.h"
#include "Srch2EngineTest.h"

inline
Srch2EngineTest* getSrch2TestFromHandle(JNIEnv *env, jlong handle) {
  Srch2EngineTest *rtn= (Srch2EngineTest*) handle;
  rtn->env= env;
  return rtn;
}

/*
 * Class:     com_srch2_Srch2EngineTest
 * Method:    createSearchableEngine
 * Signature: (Ljava/lang/Class;Ljava/lang/reflect/Method;)J
 */
JNIEXPORT jlong JNICALL
Java_com_srch2_Srch2EngineTest_createSearchableEngine(JNIEnv *env,
    jobject, jclass srch2StringClassPtr, jobject srch2StringMethodgetValue,
    jobject srch2StringConstructor) {
  
  return 
    (jlong) new Srch2EngineTest((jclass) env->NewGlobalRef(srch2StringClassPtr),
      env->FromReflectedMethod(srch2StringMethodgetValue),
      env->FromReflectedMethod(srch2StringConstructor));
}

/*
 * Class:     com_srch2_Srch2EngineTest
 * Method:    setString
 * Signature: (JLcom/srch2/SearchableString;)V
 */
JNIEXPORT void JNICALL Java_com_srch2_Srch2EngineTest_setString
  (JNIEnv *env, jobject, jlong handle, jobject string) {
  getSrch2TestFromHandle(env, handle)->setString(string);
}

/*
 * Class:     com_srch2_Srch2EngineTest
 * Method:    getString
 * Signature: (J)Lcom/srch2/SearchableString;
 */
JNIEXPORT
jobject JNICALL Java_com_srch2_Srch2EngineTest_getString (JNIEnv *env,
    jobject, jlong handle) {
  return getSrch2TestFromHandle(env, handle)->getString();
}

