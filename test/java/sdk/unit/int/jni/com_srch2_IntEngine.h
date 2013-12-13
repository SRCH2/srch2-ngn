
/*****************************************************************************
 *                                                                           *
 *              AUTHOR : RJ ATWAL                                            *
 *                                                                           *
 *                                                                           * 
  ****************************************************************************/

#include <jni.h>
/* Header for class com_srch2_IntEngine */

#ifndef __com_srch2_IntEngine__
#define __com_srch2_IntEngine__
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_srch2_IntEngine
 * Method:    createIntEngine
 * Signature: (Ljava/lang/reflect/Method;
               Ljava/lang/Class; Ljava/lang/reflect/Constructor;)J
 */
JNIEXPORT jlong JNICALL Java_com_srch2_IntEngine_createIntEngine
  (JNIEnv *, jclass, jobject, jclass, jobject);

/*
 * Class:     com_srch2_IntEngine
 * Method:    setInt
 * Signature: (JLcom/srch2/Attribute;)V
 */
JNIEXPORT void JNICALL Java_com_srch2_IntEngine_setInt
  (JNIEnv *, jclass, jlong, jobject);

/*
 * Class:     com_srch2_IntEngine
 * Method:    getSearchableInt
 * Signature: (J)Lcom/srch2/RefiningInteger;
 */
JNIEXPORT jobject JNICALL Java_com_srch2_IntEngine_getRefiningInt
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_srch2_IntEngine
 * Method:    deleteRefiningEngine
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_srch2_IntEngine_deleteIntEngine
  (JNIEnv *, jclass, jlong);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __com_srch2_IntEngine__ */

