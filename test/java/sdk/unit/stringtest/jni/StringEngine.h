#include "Srch2String.h"
#include<string>
#include<cassert>

class SearchableEngine {
 
  /** Encapsulates the instance language around a Java SearchableString object
      used by the particular JVM housing the Java side of this Engine */
  const JNIClass::SearchableString searchableString;

  /*  The stored value of this Engine */
  std::string value;

 public:
  /* Warning: this variable make this class thread unsafe, only
     a single thread my access a given instance of this class.
  
     The JVM context of the Java side of this Engine on a particular bridged
     call.
   */
  JNIEnv *env;

  Srch2EngineTest(jclass srch2StringClassPtr,
      jmethodID getValue, jmethodID constructor)
    : srch2String(this->env, srch2StringClassPtr, getValue, constructor) {}

  /* returns a Java SearchableString instance with value equivalent
     to the string contained this Engine. */
  jobject getString();
  /* Stores a new string value in this Engine, equivalent to the value of
     SearchableString given. */
  void setString(jobject);
};


