
/*****************************************************************************
 *                                                                           *
 *              AUTHOR : RJ ATWAL                                            *
 *                                                                           *
 *                                                                           * 
  ****************************************************************************/

#include "StringAttribute.h"
#include "util/utf8/unchecked.h"
#include "util/utf16/utf16.h"

std::string JNIClass::StringAttribute::toString(jobject srch2String) const {
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

jboolean JNIClass::StringAttribute::isInstance(jobject obj) const {
  return env->IsAssignableFrom(env->GetObjectClass(obj), classPtr);
}

jobject JNIClass::StringAttribute::createNew(std::string& content) const {
  jchar uft16start[1024];
  
  /* convert a UTF8 string to a UTF16 encoded array*/
  jchar *uft16end= 
    utf8::unchecked::utf8to16(content.begin(), content.end(), uft16start);

  /* Creates a new String on the Java heap */
  jstring internalString= env->NewString(uft16start, uft16end-uft16start);

  /* Creates a new Object of StringAttribute type on the java heap using
     the constructor specified with internalString as an argument.

     Runs 
         new StringAttribute(internalString)

     in the JVM, and returns a handle to the new object */
  return env->NewObject(this->classPtr, constructor, internalString);
}

