#include "Srch2String.h"
#include<string>
#include<cassert>

class Srch2EngineTest {

  const JNIClass::Srch2String srch2String;
  /* Warning: this variable make this class thread unsafe, only
     a single thread my access a given instance of this class */

  std::string value;

 public:
  JNIEnv *env;

  Srch2EngineTest(jclass srch2StringClassPtr,
      jmethodID getValue, jmethodID constructor)
    : srch2String(this->env, srch2StringClassPtr, getValue, constructor) {}

  /* returns a Java Srch2String instance with value equivalant to the contained
     string. */
  jobject getString();
  void setString(jobject);
};

jobject Srch2EngineTest::getString() {
  return srch2String.createNew(value);
}

void Srch2EngineTest::setString(jobject string) {
  assert(JNI_TRUE == srch2String.isInstance(string));
  this->value = srch2String.toString(string);
}
