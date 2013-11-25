
/*****************************************************************************
 *                                                                           *
 *              AUTHOR : RJ ATWAL                                            *
 *                                                                           *
 *                                                                           * 
  ****************************************************************************/

#include <jni.h>
/* Header for class com_srch2_FloatEngine */

#ifndef __com_srch2_FloatEngine__
#define __com_srch2_FloatEngine__
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_srch2_FloatEngine
 * Method:    createFloatEngine
 * Signature: (Ljava/lang/reflect/Method;
               Ljava/lang/Class; Ljava/lang/reflect/Constructor;)J
 */
JNIEXPORT jlong JNICALL Java_com_srch2_FloatEngine_createFloatEngine
  (JNIEnv *, jclass, jobject, jclass, jobject);

/*
 * Class:     com_srch2_FloatEngine
 * Method:    setFloat
 * Signature: (JLcom/srch2/Attribute;)V
 */
JNIEXPORT void JNICALL Java_com_srch2_FloatEngine_setFloat
  (JNIEnv *, jclass, jlong, jobject);

/*
 * Class:     com_srch2_FloatEngine
 * Method:    getRefiningFloat
 * Signature: (J)Lcom/srch2/RefiningFloat;
 */
JNIEXPORT jobject JNICALL Java_com_srch2_FloatEngine_getRefiningFloat
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_srch2_FloatEngine
 * Method:    deleteRefiningEngine
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_srch2_FloatEngine_deleteFloatEngine
  (JNIEnv *, jclass, jlong);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __com_srch2_FloatEngine__ */

