#ifndef __STRING_ATTRIBUTE_H__
#define __STRING_ATTRIBUTE_H__

/*****************************************************************************
 *                                                                           *
 *              AUTHOR : RJ ATWAL                                            *
 *                                                                           *
 *                                                                           * 
  ****************************************************************************/

#include "Attribute.h"
#include<string>

namespace JNIClass {
  /* A wrapper around the java StringAttribute class */
  struct StringAttribute : Attribute {
    StringAttribute(JNIEnv*& env, jclass classPtr, jmethodID getValue,
        jmethodID constructor)
      : Attribute(env, classPtr, getValue, constructor) {}

    /** converts a StringAttribute java instance, with the assumed encoding
        of UTF16, into a c++ string, encoded in UTF8 */
    std::string toString(jobject) const;
    /** creates a new StringAttribute java instance, encoded in UTF16,
        with the equivalant value of the given c++ string encoded in UTF8 */
    jobject createNew(std::string&) const;
  };
}

#include "util/utf8/unchecked.h"
#include "util/utf16/utf16.h"

std::string inline
JNIClass::StringAttribute::toString(jobject srch2String) const {
  jstring srch2StringValue=
   (jstring) env->CallObjectMethod(srch2String, getValue);
  const jchar *utf16CharStringValue=
    env->GetStringChars(srch2StringValue, JNI_FALSE);

  /* converts a UTF16 char array to UTF8 */
  utf8::uint16_t* utf16Start= (utf8::uint16_t*) utf16CharStringValue;
  utf8::uint16_t* utf16CharString= (utf8::uint16_t*) utf16CharStringValue;
  utf8::uint16_t* utf16End= (utf8::uint16_t*) 
    ((char*) utf16Start +
    utf16::byteLength(env->GetStringLength(srch2StringValue),
      utf16CharString));
  char utf8Start[1024];
  char *utf8End= utf8::unchecked::utf16to8(utf16Start, utf16End, utf8Start);

  return std::string(utf8Start, utf8End);
}

void inline creatNew(std::string& content, jchar* buffer,
    JNIEnv &env, jobject& internalString) {
  jchar *end;

  /* convert a UTF8 string to a UTF16 encoded array*/
  end= utf8::unchecked::utf8to16(content.begin(), content.end(), buffer);

  /* Creates a new String on the Java heap */
  internalString= (jobject) env.NewString(buffer, end - buffer);

}

jobject inline
JNIClass::StringAttribute::createNew(std::string& content) const {
  /* Checks the size of input string to see if it can be handled on the stack*/
  jchar *buffer;
  jvalue internalString;
  jobject rtn;

  if((content.length() < 512)) {
    jchar uft16start[1024];
    buffer= uft16start;
  }
  else {
    buffer= new jchar[content.length() * 2];
  }
  /* Creates a new Object of StringAttribute type on the java heap using
     the constructor specified with internalString as an argument.

     Runs 
         new StringAttribute(internalString)

     in the JVM, and returns a handle to the new object */
  creatNew(content, buffer, *(this->env), internalString.l);
  rtn= Attribute::createNew(internalString);
  
  /* free heap memory if needed */
  if((content.length() >= 512)) 
    delete buffer;

  return rtn;
}

#endif /* __STRING_ATTRIBUTE_H__ */
