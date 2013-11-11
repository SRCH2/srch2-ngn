
/*****************************************************************************
 *                                                                           *
 *              AUTHOR : RJ ATWAL                                            *
 *                                                                           *
 *                                                                           * 
  ****************************************************************************/

#include "Attribute.h"

class FloatEngine {
 
  /** Encapsulates the instance language around a Java RefiningFloat object
      used by the particular JVM housing the Java side of this Engine */
  const JNIClass::Attribute<float> refiningFloat;

  /*  The stored value of this Engine */
  float value;

 public:
  /* Warning: this variable make this class thread unsafe, only
     a single thread may access a given instance of this class.
  
     The JVM context of the Java side of this Engine on a particular bridged
     call.
   */
  JNIEnv *env;

  FloatEngine(jmethodID getValue,
      jclass refiningFloatClassPtr, jmethodID refiningConstructor)
    : refiningFloat(this->env, refiningFloatClassPtr,
          getValue, refiningConstructor) {}

  /* returns a Java RefiningFloat instance with value equivalent
     to the float contained this Engine. */
  jobject getRefiningFloat();
 
  /* Stores a new float value in this Engine, equivalent to the value of
     given FloatAttribute. */
  void setFloat(jobject);
};

