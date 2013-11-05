#include "RefiningString.h"
#include<string>
#include<cassert>

class StringEngine {
 
  /** Encapsulates the instance language around a Java RefiningString object
      used by the particular JVM housing the Java side of this Engine */
  const JNIClass::RefiningString refiningString;

  /*  The stored value of this Engine */
  std::string value;

 public:
  /* Warning: this variable make this class thread unsafe, only
     a single thread may access a given instance of this class.
  
     The JVM context of the Java side of this Engine on a particular bridged
     call.
   */
  JNIEnv *env;

  StringEngine(jclass refiningStringClassPtr,
      jmethodID getValue, jmethodID constructor)
    : refiningString(this->env, 
        refiningStringClassPtr, getValue, constructor) {}

  /* returns a Java RefiningString instance with value equivalent
     to the string contained this Engine. */
  jobject getString();
  /* Stores a new string value in this Engine, equivalent to the value of
     RefiningString given. */
  void setString(jobject);
};


