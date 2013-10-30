#include "SearchableString.h"
#include "util/utf8/unchecked.h"
#include "util/utf16/utf16.h"



std::string JNIClass::SearchableString::toString(jobject srch2String) const {
  jstring srch2StringValue=
   (jstring) env->CallObjectMethod(srch2String, getValue);
  const jchar *utf16CharStringValue=
    env->GetStringChars(srch2StringValue, JNI_FALSE);

  /* converts a utf16 char array to utf8 */
  utf8::uint16_t* utf16Start= (utf8::uint16_t*) utf16CharStringValue;
  utf8::uint16_t* utf16CharString= (utf8::uint16_t*) utf16CharStringValue;
  utf8::uint16_t* utf16End= utf16Start +
    utf16::byteLength(env->GetStringLength(srch2StringValue), utf16CharString);
  char utf8Start[1024];
  char *utf8End= utf8Start;
  utf8::unchecked::utf16to8(utf16Start, utf16End, utf8End);

  return std::string(utf8Start, utf8End);
}

jboolean JNIClass::SearchableString::isInstance(jobject obj) const {
  return env->IsAssignableFrom(env->GetObjectClass(obj), classPtr);
}

jobject JNIClass::SearchableString::createNew(std::string&) const {
  jchar uft16start[1024];
  jchar *uft16end= uft16start;
  
  env->NewString(uft16start, uft16end-uft16start);
  return env->NewObject(classPtr, constructor);
}
