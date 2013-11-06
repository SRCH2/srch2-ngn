
/*****************************************************************************
 *                                                                           *
 *              AUTHOR : RJ ATWAL                                            *
 *                                                                           *
 *                                                                           * 
  ****************************************************************************/

#include "RefiningString.h"
#include "SearchableString.h"
#include<string>
#include<cassert>

class StringEngine {
 
  /** Encapsulates the instance language around a Java RefiningString object
      used by the particular JVM housing the Java side of this Engine */
  const JNIClass::RefiningString refiningString;
  const JNIClass::SearchableString searchableString;

  /*  The stored value of this Engine */
  std::string value;

 public:
  /* Warning: this variable make this class thread unsafe, only
     a single thread may access a given instance of this class.
  
     The JVM context of the Java side of this Engine on a particular bridged
     call.
   */
  JNIEnv *env;

  StringEngine(jmethodID getValue,
      jclass refiningStringClassPtr, jmethodID refiningConstructor,
      jclass searchableStringClassPtr, jmethodID searchableConstructor)
    : refiningString(this->env, refiningStringClassPtr,
          getValue, searchableConstructor),
      searchableString(this->env, searchableStringClassPtr,
         getValue, searchableConstructor) {}

  /* returns a Java RefiningString instance with value equivalent
     to the string contained this Engine. */
  jobject getRefiningString();
  /* returns a Java SearchableString instance with value equivalent
     to the string contained this Engine. */
  jobject getSearchableString();
 
  /* Stores a new string value in this Engine, equivalent to the value of
     given StringAttribute. */
  void setString(jobject);
};

