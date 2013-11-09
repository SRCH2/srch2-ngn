
/*****************************************************************************
 *                                                                           *
 *              AUTHOR : RJ ATWAL                                            *
 *                                                                           *
 *                                                                           * 
  ****************************************************************************/

#include "Attribute.h"

class IntEngine {
 
  /** Encapsulates the instance language around a Java RefiningInteger object
      used by the particular JVM housing the Java side of this Engine */
  const JNIClass::Attribute<int> refiningInteger;

  /*  The stored value of this Engine */
  int value;

 public:
  /* Warning: this variable make this class thread unsafe, only
     a single thread may access a given instance of this class.
  
     The JVM context of the Java side of this Engine on a particular bridged
     call.
   */
  JNIEnv *env;

  IntEngine(jmethodID getValue,
      jclass refiningIntegerClassPtr, jmethodID refiningConstructor)
    : refiningInteger(this->env, refiningIntegerClassPtr,
          getValue, refiningConstructor) {}

  /* returns a Java RefiningInteger instance with value equivalent
     to the int contained this Engine. */
  jobject getRefiningInt();
 
  /* Stores a new string value in this Engine, equivalent to the value of
     given IntegerAttribute. */
  void setInt(jobject);
};

